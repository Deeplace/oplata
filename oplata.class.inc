<?php

/**
 * Oplata.md class.
 */
class Oplata
{
  private $balanceUrl = "https://oplata.md/shopxml/balance/%URLKEY";
  private $ratesUrl = "https://oplata.md/shopxml/rates/%URLKEY";
  private $statusUrl = "https://oplata.md/shopxml/status/%URLKEY";
  private $urlkey = "";
  private $key = "";
  private $oplid = "";
  private $cert = "";
  private $debug = false;
  private $logFile = "";

  protected static $_instance;

  /**
   * Closed constructor
   */
  private function __construct() {
    $this->urlkey = variable_get('oplata_urlkey', '');
    $this->key = variable_get('oplata_key', '');
    $this->oplid = variable_get('oplata_oplid', '');
    $this->cert = variable_get('oplata_cert', '');
    if ($this->debug = variable_get('oplata_debug', 0)) {
      if (!file_exists($log_path = getcwd().base_path().file_directory_path().'/oplata')) mkdir($log_path, 0777, true);
      $this->logFile = getcwd().base_path().file_directory_path().'/oplata/'.date('Y-m-d').'.log';
      file_put_contents(getcwd().base_path().file_directory_path().'/oplata/.htaccess', 'Deny from all');
    }
    $this->balanceUrl = str_replace('%URLKEY', $this->urlkey, $this->balanceUrl);
    $this->ratesUrl = str_replace('%URLKEY', $this->urlkey, $this->ratesUrl);
    $this->statusUrl = str_replace('%URLKEY', $this->urlkey, $this->statusUrl);

    return $this;
  }

  /**
   * Closed clone method
   */
  private function __clone() {}

  /**
   * Instance method
   *
   * @return Oplata
   */
  public static function inst() {
    if ( is_null(self::$_instance) ) {
      self::$_instance = new self;
    }
    return self::$_instance;
  }

  /**
   * Make hash value
   *
   * @param  mixed $value
   * @return string
   */
  public function hash($value) {
    $hash = $this->oplid.':'.$this->key.':'.$this->urlkey;
    if (is_array($value)) $hash = implode(':', $value).':'.$hash;
    return strtoupper(md5($hash));
  }

  /**
   * Make request and fetch data from Oplata.md gate
   *
   * @param  string $xml
   * @return array
   */
  public function fetch($url, $xml) {
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER,1);
    curl_setopt($ch, CURLOPT_HEADER,0);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: text/xml; charset=utf-8', 'Content-length: '.strlen($xml)));
    curl_setopt($ch, CURLOPT_POST,1);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, true);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
    curl_setopt($ch, CURLOPT_CAINFO, getcwd().base_path().$this->cert);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $xml);
    $response = curl_exec($ch);
    curl_close($ch);

    if ($this->debug) {
      ob_start();
      $fp = fopen($this->logFile, 'a+');
      echo "url:\n{$url}\n\n";
      echo "raw request:\n{$xml}\n\n";
      echo "raw response:\n{$response}\n\n";
      fwrite($fp, "------ [".date('Y-m-d H:i:s')."] ------\n\n".ob_get_contents()."\n\n");
      fclose($fp);
      ob_end_clean();
    }

    return simplexml_load_string($response);
  }

  /**
   * Convert request from a arrray to string
   *
   * @param  array $array
   * @return string
   */
  public function toXml($array) {
    $xml = '<?xml version="1.0" encoding="UTF-8"?>';
    $xml .= "\n<oplata_request>\n";
    if (is_array($array)){
      foreach($array as $tag => $value) $xml .= "<$tag>$value</$tag>\n";
    }
    $xml .= "</oplata_request>";

    return $xml;
  }

  /**
   * Get balance from Oplata.md
   *
   * @return mixed
   */
  public function getBalance() {
    $rand = time().mt_rand(1000,99999);
    $hash = $this->hash($rand);
    $array = array('login' => $this->oplid, 'qid' => $rand, 'sign' => $hash);
    $xml = $this->toXml($array);
    $response = $this->fetch($this->balanceUrl, $xml);
    foreach($response as $tag => $value){
      if ($tag=='balance_lei') return $value;
    }
    return 0;
  }

  /**
   * Get rates from Oplata.md
   */
  public function getRates() {
    $rand = time().mt_rand(1000,99999);
    $hash = $this->hash(array($rand));
    $array = array('login' => $this->oplid, 'qid' => $rand, 'sign' => $hash);
    $xml = $this->toXml($array);
    $response = $this->fetch($this->ratesUrl, $xml);
    $response = (array)$response;
    $return = array();
    if (is_object($response['systems'])) {
      $rate = (array)$response['systems'];
      $return = array($rate['id'] => $rate);
    } elseif ($response['systems']) {
      foreach ($response['systems'] as $rate) {
        $rate = (array)$rate;
        $return[$rate['id']] = $rate;
      }
    }

    return $return;
  }

  /**
   * Get transaction status from Oplata.md
   *
   * @param  int $account
   * @param  float $amount
   * @return int (2 - success, 1 - not confirmed, other - not found, -1 - signature error)
   */
  public function getStatus($account, $amount) {
    $rand = time().mt_rand(1000,99999);
    $amount = number_format($amount, 2, '.', '');
    $arrayHash = array($rand, $account, $amount);
    $hash = $this->hash($arrayHash);
    $array = array('login' => $this->oplid, 'account' => $account, 'amount' => $amount, 'qid' => $rand, 'sign' => $hash);
    $xml = $this->toXml($array);
    $response = $this->fetch($this->statusUrl, $xml);
    $response = (array)$response;

    foreach ($response as $key => $value) {
      if ($key != 'signature') {
        $arrayHash2[] = $value;
        $oplata_array[$key] = $value;
      } else $oplata_signature = $value;
    }
    $hash = $this->hash($arrayHash2);
    if ($hash == $oplata_signature) {
      return $oplata_array['status'];
    }
    return '-1';
  }
}