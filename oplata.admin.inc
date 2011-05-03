<?php

/**
 * @file
 * Oplata.md admin form.
 */

/**
 * Oplata.md module default settings form.
 */
function oplata_settings() {
  $form = array();

  $form['oplata_urlkey'] = array(
    '#type' => 'textfield',
    '#required' => true,
    '#title' => t('URLKEY'),
    '#default_value' => variable_get('oplata_urlkey', '')
  );

  $form['oplata_key'] = array(
    '#type' => 'textfield',
    '#required' => true,
    '#title' => t('KEY'),
    '#description' => t('The key of your shop.'),
    '#default_value' => variable_get('oplata_key', '')
  );

  $form['oplata_oplid'] = array(
    '#type' => 'textfield',
    '#required' => true,
    '#title' => t('OPLID'),
    '#description' => t('The number of your shop.'),
    '#default_value' => variable_get('oplata_oplid', '')
  );

  $form['oplata_cert'] = array(
    '#type' => 'textfield',
    '#required' => true,
    '#title' => t('Path to certificat-file'),
    '#description' => t('The relative path to the certificat file.'),
    '#default_value' => variable_get('oplata_cert', '')
  );

  $response = Oplata::inst()->getRates();
  $currency = array();
  foreach ($response as $id => $rate) {
    $currency[$id] = $rate['currency'];
  }
  $form['oplata_currency'] = array(
    '#type' => 'select',
    '#title' => t('Currency'),
    '#options' => $currency,
    '#default_value' => variable_get('oplata_currency', 5)
  );

  $form['oplata_granulity'] = array(
    '#type' => 'textfield',
    '#title' => t('Granulity'),
    '#description' => t('Amount granulity. Possible only decimal values. Values must be separated by ";".'),
    '#default_value' => variable_get('oplata_granulity', "5;10;20;50;75;100")
  );

  $form['oplata_debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug mode'),
    '#description' => t('If enabled the debug data can be saved into the log-files.'),
    '#default_value' => variable_get('oplata_debug', 0)
  );

  $form['oplata_success_message'] = array(
    '#type' => 'textarea',
    '#title' => t('Transaction success message'),
    '#description' => t('Customize the success message. Allowable variables: !shopdata, !qid, !account, !status, !amount, !transid.'),
    '#default_value' => variable_get('oplata_success_message', "Transaction succeed!

Transaction Id: !transid
Transaction number: !account
Amount: !amount
Oplata.md shop data: !shopdata")
  );

  $form['oplata_error_message'] = array(
    '#type' => 'textarea',
    '#title' => t('Transaction error message'),
    '#description' => t('Customize the error message. Allowable variables: !status.'),
    '#default_value' => variable_get('oplata_error_message', 'Transaction returns error code.')
  );

  $form['oplata_confirm_error_message'] = array(
    '#type' => 'textarea',
    '#title' => t('Transaction not confirmed message'),
    '#default_value' => variable_get('oplata_confirm_error_message', 'The transaction not confirmed.')
  );

  $form['oplata_sign_error_message'] = array(
    '#type' => 'textarea',
    '#title' => t('Transaction signature error message'),
    '#default_value' => variable_get('oplata_signature_error_message', 'Signature error! Please contact the site administrator.')
  );

  return system_settings_form($form);
}

/**
 * Implementation of hook_validate().
 */
function oplata_settings_validate($form, &$form_state) {
  if (!file_exists($file = getcwd().base_path().$form_state['values']['oplata_cert'])) {
    form_set_error('oplata_cert', t('The certification file %file does not exists.', array('%file' => $file)));
  }
}