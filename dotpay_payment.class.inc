<?php
/**
 * This file contains classes, properties and methods for the Dotpay Payment module.
 */
// TODO extend status class and add new statuses
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
    
    $controller_data = $payment->method->controller_data;
    
    $dotpay_pin = $controller_data['dotpay_pin'];
    
    drupal_goto('payment/dotpay/form/' . $payment->pid);
  }
  
  /**
   * 
   */
  public function form(Payment $payment, $form, &$form_state) {
    global $base_url;
    global $language;

    $controller_data = $payment->method->controller_data;
    $channels = $payment->method->controller->channelOptions();
dsm($payment->pid);
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
    $account = $GLOBALS['user'];

    dsm($account);
    $payment_data = array(
      'amount' => $amount,
      'description' => $payment->description,
    );

    /*
     * 
     */
    $data = array(
      'id' => $controller_data['dotpay_id'],
      'amount' => number_format($payment_data['amount'], 2, '.', ''),
      'description' => $payment_data['description'],
      'p_email' => variable_get('site_mail', ''),
      'email' => ($account->mail)? $account->mail : '',
      'currency' => $currency,
      'lang' => $language->language,
      'control' => $payment->pid,
      'ch_lock' => $controller_data['dotpay_ch_lock'],
      'onlinetransfer' => $controller_data['dotpay_onlinetransfer'],
      'type' => 0,
      'URL' => url(DOTPAY_PAYMENT_RETURN_PATH
              . '/' . $payment->pid, array('absolute' => TRUE)),
      'URLC' => url(DOTPAY_PAYMENT_RETURN_PATH
              . '/' . $payment->pid, array('absolute' => TRUE)),
    );
    $form['#action'] = $url;
    
    foreach($data as $name => $value) {
      if (!empty($value)) {
        $form[$name] = array('#type' => 'hidden', '#value' => $value);
      }
    }
    // only for tests - it will be removed soon
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
        $options = array_merge($options, array($value['channel'] => $value['title']));
      }
    }
    //$form['c_email'] = array(
      //'type' => 'textfield',
      //'#title' => t('Email'),
      //'#required' => TRUE,
      //);
    $form['channel'] = array(
      '#type' => 'radios',
      '#options' => $options,
      );
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Pay with Dotpay'),
    );

    return $form;
  }
  /**
   * Function to check if have menu access
   * @param  string $value1
   * @param  string $value2
   * @param  string $message Drupal error message
   * @return constant MENU_ACCESS_DENIED
   */
  public function checkMenuAccess($value1, $value2, $message='You have no access to this page') {
    if ($value1 != $value2) {
      drupal_set_message(t($message), 'error');
      return MENU_ACCESS_DENIED;
    }
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
            array('channel' => -1, 'title' => t('VISA, MasterCard, JCB, Diners Club')),
          ),
        ),
        'transfer_channels' => array(
          'header' => array('title' => t('Transfer channels')),
          'options' => array(
            array('channel' => 1, 'title' => t('mTransfer (mBank)')),
            array('channel' => 2, 'title' => t('I pay with Inteligo (konto Inteligo)')),
            array('channel' => 3, 'title' => t('Multitransfer (Multibank)')),
            array('channel' => 4, 'title' => t('I pay with iPKO')),
            array('channel' => 6, 'title' => t('Przelew24 (BZBWK)')),
            array('channel' => 17, 'title' => t('Pay with Nordea')),
            array('channel' => 18, 'title' => t('Przelew z BPH')),
            array('channel' => 36, 'title' => t('Pekao24Przelew')),
            array('channel' => 38, 'title' => t('I pay with ING Bank Śląski')),
            array('channel' => 44, 'title' => t('Millenium - internet payments')),
            array('channel' => 45, 'title' => t('I pay with Alior Bank')),
            array('channel' => 46, 'title' => t('I pay with City Handlowy')),
            array('channel' => 48, 'title' => t('r-przelew Raiffeisen Bank')),
            array('channel' => 49, 'title' => t('MeritumBank Przelew')),
            array('channel' => 50, 'title' => t('Pay Way Toyota Bank')),
            array('channel' => 51, 'title' => t('Pay with BOŚ')),
            array('channel' => 56, 'title' => t('Eurobank')),
            array('channel' => 58, 'title' => t('Szybkie Płatności Internetowe z Deutsche Bank PBC')),
            array('channel' => 60, 'title' => t('Alior Sync')),
          ), 
        ),
        'nontransfer_channels' => array(
          'header' => array('title' => t('Non-transfer channels')),
          'options' => array(
            array('channel' => 7, 'title' => t('ING Bank Śląski')),
            array('channel' => 8, 'title' => t('SEZAM (Bank BPH S.A.)')),
            array('channel' => 9, 'title' => t('ING Bank Śląski')),
            array('channel' => 10, 'title' => t('MilleNet (Millenium Bank)')),
            array('channel' => 13, 'title' => t('Deutsche Bank PBC S.A.')),
            array('channel' => 14, 'title' => t('Kredyt Bank S.A. (KB24)')),
            array('channel' => 15, 'title' => t('iPKO (Bank PKO BP)')),
            array('channel' => 16, 'title' => t('Credit Agricole Bank Polska')),
            array('channel' => 19, 'title' => t('CitiBank Handlowy')),
            array('channel' => 25, 'title' => t('InvestBank')),
            array('channel' => 26, 'title' => t('Bank Ochrony Środowiska')),
            array('channel' => 27, 'title' => t('Bank Gospodarki Żywnościowej')),
            array('channel' => 32, 'title' => t('BNP Paribas Fortis')),
            array('channel' => 33, 'title' => t('Volkswagen bank Polska S.A.')),
            array('channel' => 42, 'title' => t('Alior Bank')),
            array('channel' => 43, 'title' => t('BS Wschowa')),
            array('channel' => 47, 'title' => t('Polbank EFG')),
            array('channel' => 57, 'title' => t('Getin Bank')),
            array('channel' => 61, 'title' => t('Bank Pocztowy')),
            array('channel' => 62, 'title' => t('DnB Nord')),
          ),
        ),
        'cash_channels' => array(
          'header' => array('title' => t('Cash channels')),
          'options' => array(
            array('channel' => 11, 'title' => t('Bank, post office transfer')),
            array('channel' => 21, 'title' => t('Moje Rachunki')),
            array('channel' => 31, 'title' => t('Pay in Żabka')),
            array('channel' => 35, 'title' => t('Kantor Polski')),
            array('channel' => 41, 'title' => t('TransKasa')),
          ),
        ),
        'virtual_wallet' => array(
          'header' => array('title' => t('Virtual Wallet')),
          'options' => array(
            array('channel' => 24, 'title' => t('mPay - mobile payment')),
            array('channel' => 52, 'title' => t('SkyCash')),
            array('channel' => 63, 'title' => t('I pay with iPKO')),
            array('channel' => 212, 'title' => t('PayPal')),
          ),
        ),
        'others' => array(
          'header' => array('title' => t('Others')),
          'options' => array(
            array('channel' => 22, 'title' => t('Ukash Coupon')),
            array('channel' => 55, 'title' => t('E-rata - zakupy na raty z Dotpay')),
          ),
        ),
      );
    }
    return $map;
  }
  
/**
   * Returns Dotpay payment method brand options.
   *
   * @return array
   *   Keys are payment method/brand names. Values array arrays that contain brand names.
   *
*/
  public function channelOptions() {
    $map = $this->channels();
    /*$options = array();
    foreach ($map as $dotpay_payment_method => $dotpay_payment_method_brands) {
      foreach ($dotpay_payment_method_brands as $brand) {
        $options[$dotpay_payment_method . ':' . $brand] = $brand;
      }
    }*/
    return $map;
  }
  
}