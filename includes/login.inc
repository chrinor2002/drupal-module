<?php
/**
 * @file
 * Page for inputting GatherContent authentication settings.
 */

/**
 * Form constructor for displaying login credentials.
 */
function gathercontent_login_form($form, &$form_state) {
  gathercontent_check_step('login');
  $form['gathercontent_api_url'] = array(
    '#type' => 'textfield',
    '#default_value' => variable_get('gathercontent_api_url', ''),
    '#required' => TRUE,
    '#prefix' => '<div class="form-item form-type-textfield form-item-gathercontent-api-url"><label for="edit-gathercontent-api-url">' . t('Your login URL') . '</label><span class="gc_url_prefix">https://</span>',
    '#suffix' => '<span class="gc_url_suffix">.gathercontent.com</span><div class="description">' . t('Enter the first part of the GatherContent URL when you\'re logged in, eg. https://[accountname].gathercontent.com.') . '</div></div>',
    '#theme_wrappers' => array(),
  );

  $form['gathercontent_api_key'] = array(
    '#type' => 'textfield',
    '#default_value' => variable_get('gathercontent_api_key', ''),
    '#required' => TRUE,
    '#prefix' => '<div class="form-item form-type-textfield form-item-gathercontent-api-key"><label for="edit-gathercontent-api-key">' . t('API Key') . '</label>',
    '#suffix' => '<div class="description">' . t('You can find your unique API key inside your GatherContent account by going to Personal Settings and opening the API tab.') . '</div></div>',
    '#theme_wrappers' => array(),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Connect account'),
  );

  return $form;
}

/**
 * Form submission handler for gathercontent_login_form().
 */
function gathercontent_login_form_submit($form, &$form_state) {
  $obj = gathercontent_get_obj();
  $ret = $obj->get('get_me', array(), $form_state['values']['gathercontent_api_url'], $form_state['values']['gathercontent_api_key']);
  variable_set('gathercontent_api_url', $form_state['values']['gathercontent_api_url']);
  variable_set('gathercontent_api_key', $form_state['values']['gathercontent_api_key']);
  if ($ret === FALSE) {
    drupal_set_message($obj->error, 'error');
  }
  else {
    drupal_goto('admin/config/content/gathercontent');
  }
}
