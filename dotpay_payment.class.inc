<?php
/**
 * This file contains classes, properties and methods for the Dotpay Payment module.
 */

/**
 * Build Dotpay payment method controller.
 */
class DotpayPaymentMethodController extends PaymentMethodController {

  public $payment_method_configuration_form_elements_callback = 'dotpay_payment_method_configuration';
  // object contructor
  function __construct() {
    $this->title = t('Dotpay');
  }

  /**
   * Implements PaymentMethodController::execute().
   */
  public function execute(Payment $payment) {
    $salt = md5($payment->pid . drupal_get_hash_salt());
    drupal_goto('payment/dotpay/form/' . $payment->pid . '/' . $salt);
  }
  
  /**
   * 
   */
  public function form(Payment $payment, $form, &$form_state) {
    global $base_url;
    global $language;

    $controller_data = $payment->method->controller_data;
    $channels = $payment->method->controller->channelOptions();

    // Get settings from controller data.
    $dotpay_id = $controller_data['dotpay_id'];
    $dotpay_pin = $controller_data['dotpay_pin'];
    // URL 
    $url = $controller_data['dotpay_url'];
    $currency = $payment->currency_code;
    
    /**
     * Calculate amount to pay.
     */
    $amount = 0;
    foreach ($payment->line_items as $line_item) {
      $amount += (1 + $line_item->tax_rate) * $line_item->amount
              * $line_item->quantity;
    }
    // Get 
    $account = $GLOBALS['user'];

    $payment_data = array(
      'amount' => $amount,
      'description' => $payment->description,
    );
    $form['email'] = array(
      '#type' => 'textfield',
      '#title' => t('Email'),
      '#default_value' => $account->mail ? $account->mail : '',
      '#required' => TRUE,
      '#description' => t('Insert your email address'),
      );
    /*
     * Build $data array with payment details
     */
    $data = array(
      'id' => $controller_data['dotpay_id'],
      'amount' => number_format($payment_data['amount'], 2, '.', ''),
      'description' => $payment_data['description'],
      'p_email' => variable_get('site_mail', ''),
      'currency' => $currency,
      'lang' => $language->language,
      'control' => $payment->pid,
      'onlinetransfer' => $controller_data['dotpay_onlinetransfer'],
      'ch_lock' => $controller_data['dotpay_ch_lock'],
      'type' => 0,
      'URL' => url($controller_data['dotpay_return_url'], array('absolute' => TRUE)),
      'URLC' => url(DOTPAY_PAYMENT_RETURN_PATH
              . '/' . $payment->pid, array('absolute' => TRUE)),
    );
    $form['#action'] = $url;
    
    foreach($data as $name => $value) {
      if (!empty($value)) {
        $form[$name] = array('#type' => 'hidden', '#value' => $value);
      }
    }
    
    $dotpay_channels = $controller_data['dotpay_channels'];
    $options_keys = array();
    foreach ($dotpay_channels as $channels_group => $channels_ids) {
      $options_keys = array_merge($options_keys, array_filter($channels_ids));
    }
    $options_values = array();
    foreach ($channels as $channel_group => $channel) {
      $options_values = array_merge($options_values, $channel['options']);
    }
    $options = array();
    foreach ($options_values as $option => $value) {
      if (in_array($value['channel'], $options_keys)) {
        if ($value['channel'] == '-1') {
          $options = $options + array(0 => $value['title']);
        } else {
          $options = $options + array($value['channel'] => $value['title']);
        }
      }
    }  
    $form['channel'] = array(
      '#type' => 'radios',
      '#options' => $options,
      );
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Pay with Dotpay'),
    );
    dsm($form);
    return $form;
  }

  /**
   * Function checking if given string is valid md5 hash
   * @param  string  $md5 string to check
   * @return boolean      true/false ansver
   */
  public function isValidMd5($md5 ='') {
    return strlen($md5) == 32 && ctype_xdigit($md5);
  }
    /**
   * @todo Write function documentation.
   */
  public function generate_md5($pin, $id, $data) {
    return $this->_generate_signature($pin, $id, $data);
  }

  /**
   * Private method to generate md5 hash
   * @param  string $pin  dotpay secret PIN
   * @param  integer $id  payment ID
   * @param  array $data  dotpay form parameters
   * @return md5 hash    
   */
  private function _generate_md5($pin, $id, $data) {
    return md5(
      $pin . ':' .
      $id . ':' .
      isset($data['control']) ? $data['control']: '' .':'.
      isset($data['t_id']) ? $data['t_id']: '' . ':' .
      isset($data['amount']) ? $data['amount']: '' . ':' .
      isset($data['email']) ? $data['email']: '' . ':' .
      isset($data['service']) ? $data['service']: '' . ':' .
      isset($data['code']) ? $data['code']: '' . ':' .
      isset($data['username']) ? $data['username']: '' . ':' .
      isset($data['password']) ? $data['password']: '' . ':' .
      isset($data['t_status']) ? $data['t_status']: ''
    );

  }
  
  /**
   * Return payment statuses
   * @param  $dotpay_status   this is the status code
   * @return constant
   */
  public function status($dotpay_status) {
    $payment_status = PAYMENT_STATUS_PENDING;
    switch ($dotpay_status) {
      case 1:
      // new
        $payment_status = PAYMENT_STATUS_NEW;
        break;
      case 2:
      // completed
        $payment_status = PAYMENT_STATUS_SUCCESS;
        break;
      case 3:
      // refused
        $payment_status = PAYMENT_STATUS_FAILED;
        break;
      case 4:
      // canceled
        $payment_status = PAYMENT_STATUS_CANCELLED;
        break;
      case 5:
      // reklamacja
        $payment_status = PAYMENT_STATUS_CANCELLED;
        break;
    }

    return $payment_status;
  }



  /**
   * Maps payment methods and brands.
   *
   * @return array
   *   Keys are payment method names. Values array arrays that contain brand names.
  */ 
  private function channels() {
    static $map = NULL;

    if (is_null($map)) {
      $map = array(
        'credit_cards' => array(
          'header' => array('title' => t('Credit Cards')),
          'options' => array(
            array('channel' => 00, 'title' => t('VISA, MasterCard, JCB, Diners Club'), 'info' => t('Only for firm account')),
            array('channel' => 245, 'title' => t('MasterCard Mobile'),  'info' => t('Only for firm account')),
            array('channel' => 69, 'title' => t('V.me'), 'info' => t('Only for firm account')),
            array('channel' => 71, 'title' => t('MasterPass'), 'info' => t('Only for firm account')),
          ),
        ),
        'transfer_channels' => array(
          'header' => array('title' => t('Transfer channels')),
          'options' => array(
            array('channel' => 1, 'title' => t('mTransfer (mBank)'), 'info' => t('Standard account')),
            array('channel' => 2, 'title' => t('Inteligo (konto Inteligo)'), 'info' => t('Standard account')),
            array('channel' => 3, 'title' => t('Multitransfer (Multibank)'), 'info' => t('Standard account')),
            array('channel' => 4, 'title' => t('Płacę z iPKO'), 'info' => t('Standard account')),
            array('channel' => 6, 'title' => t('Przelew24 (BZBWK)'), 'info' => t('Standard account')),
            array('channel' => 17, 'title' => t('Płacę z iPKONET'), 'info' => t('Standard account')),
            array('channel' => 18, 'title' => t('Przelew z BPH'), 'info' => t('Standard account')),
            array('channel' => 36, 'title' => t('Pekao24Przelew'), 'info' => t('Standard account')),
            array('channel' => 38, 'title' => t('Płać z ING'), 'info' => t('Standard account')),
            array('channel' => 44, 'title' => t('Millenium - Płatności Internetowe'), 'info' => t('Standard account')),
            array('channel' => 45, 'title' => t('Alior Bank'), 'info' => t('Standard account')),
            array('channel' => 46, 'title' => t('Płacę z City Handlowy'), 'info' => t('Standard account')),
            array('channel' => 48, 'title' => t('R-Przelew'), 'info' => t('Standard account')),
            array('channel' => 49, 'title' => t('MeritumBank Przelew'), 'info' => t('Standard account')),
            array('channel' => 50, 'title' => t('Pay Way Toyota Bank'), 'info' => t('Standard account')),
            array('channel' => 51, 'title' => t('Płać z BOŚ'), 'info' => t('Standard account')),
            array('channel' => 56, 'title' => t('eurobank - płatność online'), 'info' => t('Standard account')),
            array('channel' => 58, 'title' => t('Szybkie Płatności Internetowe z Deutsche Bank PBC'), 'info' => t('Standard account')),
            array('channel' => 60, 'title' => t('Płacę z T-Mobile Usługi Bankowe'), 'info' => t('Standard account')),
            array('channel' => 63, 'title' => t('Płacę i IKO'), 'info' => t('Standard account')),
            array('channel' => 64, 'title' => t('PeoPay'), 'info' => t('Standard account')),
            array('channel' => 65, 'title' => t('Płacę z Idea Bank'), 'info' => t('Standard account')),
          ), 
        ),
        'nontransfer_channels' => array(
          'header' => array('title' => t('Non-transfer channels')),
          'options' => array(
            array('channel' => 7, 'title' => t('ING Klienci korporacyjni'), 'info' => t('Standard account')),
            array('channel' => 10, 'title' => t('Millenium Klienci korporacyjni'), 'info' => t('Standard account')),
            array('channel' => 14, 'title' => t('Kredyt Bank S.A. (KB24)'), 'info' => t('Standard account')),
            array('channel' => 15, 'title' => t('iPKO (Bank PKO BP)'), 'info' => t('Standard account')),
            array('channel' => 16, 'title' => t('Credit Agricole Bank Polska'), 'info' => t('Standard account')),
            array('channel' => 25, 'title' => t('Plus Bank'), 'info' => t('Standard account')),
            array('channel' => 27, 'title' => t('Bank Gospodarki Żywnościowej'), 'info' => t('Standard account')),
            array('channel' => 32, 'title' => t('BNP Paribas Fortis'), 'info' => t('Standard account')),
            array('channel' => 33, 'title' => t('Volkswagen Bank Polska'), 'info' => t('Standard account')),
            array('channel' => 43, 'title' => t('Bank Spółdzielczy we Wschowie'), 'info' => t('Standard account')),
            array('channel' => 57, 'title' => t('Getin Bank'), 'info' => t('Standard account')),
            array('channel' => 61, 'title' => t('Bank Pocztowy'), 'info' => t('Standard account')),
            array('channel' => 62, 'title' => t('DnB Nord'), 'info' => t('Standard account')),
          ),
        ),
        'cash_channels' => array(
          'header' => array('title' => t('Cash channels')),
          'options' => array(
            array('channel' => 11, 'title' => t('Bank, post office transfer'), 'info' => t('Standard account')),
            array('channel' => 21, 'title' => t('VIA - Moje Rachunki'), 'info' => t('Standard account')),
            array('channel' => 31, 'title' => t('Zapłać w Żabce i we Freshmarket'), 'info' => t('Standard account')),
            array('channel' => 35, 'title' => t('Kantor Polski'), 'info' => t('Standard account')),
          ),
        ),
        'virtual_wallet' => array(
          'header' => array('title' => t('Virtual Wallet')),
          'options' => array(
            array('channel' => 22, 'title' => t('Ukash'), 'info' => t('Standard account')),
            array('channel' => 24, 'title' => t('mPay - mobile payment'), 'info' => t('Standard account')),
            array('channel' => 52, 'title' => t('SkyCash'), 'info' => t('Standard account')),            
          ),
        ),
        'others' => array(
          'header' => array('title' => t('Others')),
          'options' => array(
            array('channel' => 212, 'title' => t('PayPal'), 'info' => t('Only for PLN currency')),
            array('channel' => 20, 'title' => t('Test channel'), 'info' => t('Channel providing test functionality, active only in test mode, remeber to disable on production.')),
          ),
        ),
        'installments' => array(
          'header' => array('title' => t('Installments')),
          'options' => array(
            array('channel' => 55, 'title' => t('E-rata - zakupy na raty z Dotpay'), 'info' => t('Only for firm account')),
            array('channel' => 68, 'title' => t('mRaty'), 'info' => t('Only for firm account')),
          ),
        ),
      );
    }
    return $map;
  }
  
/**
   * Returns Dotpay payment method brand options.
   * TODO create foreach loop to build ready channels set for next tasks
   * @return array
   *   Keys are payment method/brand names. Values array arrays that contain brand names.
   *
*/
  public function channelOptions() {
    $map = $this->channels();
    return $map;
  }
}