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

  $form['oplata_ssl_version'] = array(
    '#type' => 'select',
    '#required' => true,
    '#title' => t('SSL version'),
    '#options' => array(2=>2,3=>3,),
    '#default_value' => variable_get('oplata_ssl_version', 3)
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

  $form['oplata_checkout_user_guide'] = array(
    '#type' => 'textarea',
    '#title' => t('Checkout user guide'),
    '#default_value' => variable_get('oplata_checkout_user_guide', 'Checkout user guide

Do not close the page until confirmation of payment.'),
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

/**
 * Menu callback: manage the queue.
 */
function oplata_queue() {
  $header = array(
    array('data' => t('ID'), 'field' => 'q.qid'),
    array('data' => t('User'), 'field' => 'u.uid'),
    array('data' => t('Amount'), 'field' => 'q.amount'),
    array('data' => t('Transaction ID')),
    array('data' => t('Created'), 'field' => 'q.created', 'sort' => 'desc'),
    array('data' => t('status'), 'field' => 'q.status'),
    array('data' => t('Operations'), 'colspan' => 2),
  );

  $query = "SELECT q.*, u.uid, u.name FROM {oplata_queue} q
INNER JOIN {users} u ON u.uid=q.uid";
  $tablesort = tablesort_sql($header);
  $result = pager_query($query . $tablesort, 50);
  $rows = array();
  while ($row = db_fetch_object($result)) {
    $rows[] = array(
      $row->qid,
      theme('username', $row),
      $row->amount,
      $row->transid,
      $row->created,
      $row->status,
      $row->status=='wait' ? l(t('complete'), "admin/reports/oplata/{$row->qid}/complete") : '',
      $row->status=='wait' ? l(t('emulate'), "admin/reports/oplata/{$row->qid}/emulate") : '',
    );
  }

  $output = theme('table', $header, $rows);
  $output .= theme('pager', 50);

  return $output;
}

/**
 * Menu callback: proceed the transaction.
 */
function oplata_transaction_proceed($form_state, $qid, $op) {
  $form = array();

  $form['transid'] = array(
    '#type' => 'textfield',
    '#title' => t('Transaction ID'),
    '#maxlength' => 6,
    '#required' => true,
  );

  $form['qid'] = array('#type' => 'hidden', '#value' => $qid);
  $form['opt'] = array('#type' => 'hidden', '#value' => $op);

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * Implementation of hook_submit().
 */
function oplata_transaction_proceed_submit($form, &$form_state) {
  foreach ($form_state['values'] as $var => $value) $$var = $value;

  db_query("UPDATE {oplata_queue} SET transid='%s' WHERE qid=%d", $transid, $qid);
  $row = db_fetch_object(db_query("SELECT * FROM {oplata_queue} WHERE qid=%d", $qid));

  if ($opt == 'emulate') {
    $check = Oplata::inst()->getStatus($row->qid, $row->amount);
    switch ($check) {
      case '2':
        db_query("UPDATE {oplata_queue} SET transid=%d, status='ok' WHERE qid=%d", $row->transid, $row->qid);
        user_deposit_incoming($row->amount, 'Oplata.md', $row->qid, $row->uid);

        drupal_set_message(t(variable_get('oplata_success_message', "Transaction succeed!

Transaction Id: !transid
Transaction number: !account
Amount: !amount
Oplata.md shop data: !shopdata"), array(
          '!shopdata' => t('N/A'),
          '!account' => $row->qid,
          '!amount' => $row->amount,
          '!transid' => $row->transid,
        )));
        break;

      case '1':
        db_query("UPDATE {oplata_queue} SET transid=%d, status='fail' WHERE qid=%d", $row->transid, $row->qid);
        drupal_set_message(t(variable_get('oplata_confirm_error_message', 'The transaction not confirmed.')), 'error');
        break;

      case '-1':
        drupal_set_message(t(variable_get('oplata_signature_error_message', 'Signature error! Please contact the site administrator.')), 'error');
        break;
    }
  } elseif ($opt == 'complete') {
    db_query("UPDATE {oplata_queue} SET status='ok' WHERE qid=%d", $qid);
    user_deposit_incoming($row->amount, 'Oplata.md', $row->qid, $row->uid);
    drupal_set_message(t('The transaction has been completed.'));
  }

  drupal_goto('admin/reports/oplata');
}