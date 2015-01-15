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

    $hash = md5($payment->pid . $dotpay_pin);
    
    drupal_goto('payment/dotpay/form/' . $payment->pid . '/' . $hash);
  }
  
  /**
   * 
   */
  public function form(Payment $payment, $form, &$form_state) {
    global $base_url;

    $controller_data = $payment->method->controller_data;

    // Get settings from controller data.
    $dotpay_id = $controller_data['dotpay_id'];
    $dotpay_pin = $controller_data['dotpay_pin'];
    // URL 
    $url = $controller_data['dotpay_url'];
    $currency = $payment->currency_code;
    $onlinetransfer = $controller_data['dotpay_onlinetransfer'];
    // Pass control parameter from an URL
    $this->isValidMd5(arg(4))?$control = arg(4):dsm('Blad');
    
    /**
     * Calculate amount to pay.
     */
    $amount = 0;
    foreach ($payment->line_items as $line_item) {
      $amount += (1 + $line_item->tax_rate) * $line_item->amount
              * $line_item->quantity;
    }

    $payment_data = array(
      'amount' => $amount,
      'description' => $payment->description,
    );

    /*
     * 
     */
    $data = array(
      'id' => $dotpay_id,
      'amount' => number_format($payment_data['amount'], 2, '.', ''),
      'description' => $payment_data['description'],
      'p_email' => '', // get emails
      'email' => '',// get emails
      'currency' => $currency,
      'lang' => 'pl',
      'control' => $control, // back to payment ID
      'ch_lock' => 1, //get that value from $controller_data
      //'channel' => 1,
      'onlinetransfer' => $onlinetransfer,
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
    $options = array();
    foreach ($dotpay_channels as $channels_group => $channels) {
      $options = array_merge($options,array_filter($channels));
    }
    $form['channel'] = array(
      '#type' => 'radios',
      '#default_value' => 0,
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
            array('channel' => 2, 'title' => t('Płacę z Inteligo (konto Inteligo)')),
            array('channel' => 3, 'title' => t('Multitransfer (Multibank)')),
            array('channel' => 4, 'title' => t('Płacę z iPKO')),
            array('channel' => 6, 'title' => t('Przelew24 (BZBWK)')),
            array('channel' => 17, 'title' => t('Płać z Nordea')),
            array('channel' => 18, 'title' => t('Przelew z BPH')),
            array('channel' => 36, 'title' => t('Pekao24Przelew')),
            array('channel' => 38, 'title' => t('Płacę z ING Bank Śląski')),
            array('channel' => 44, 'title' => t('Millenium - płatności interenetowe')),
            array('channel' => 45, 'title' => t('Płacę z Alior Bankiem')),
            array('channel' => 46, 'title' => t('Płacę z City Handlowy')),
            array('channel' => 48, 'title' => t('r-przelew Raiffeisen Bank')),
            array('channel' => 49, 'title' => t('MeritumBank Przelew')),
            array('channel' => 50, 'title' => t('Pay Way Toyota Bank')),
            array('channel' => 51, 'title' => t('Płać z BOŚ')),
            array('channel' => 56, 'title' => t('Eurobank')),
            array('channel' => 58, 'title' => t('Szybkie Płatności Internetowe z Deutsche Bank PBC')),
            array('channel' => 60, 'title' => t('Alior Sync'))
          ), 
        ),
        'nontransfer_channels' => array(
          'header' => array('title' => t('Non-transfer channels')),
          'options' => array(
            array('channel' => 7, 'title' => t('ING Bank Śląski')),
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