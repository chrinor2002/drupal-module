<?php
/**
 * @file
 * Contains finished dialog.
 */

/**
 * Form constructor for displaying finished message.
 */
function gathercontent_finished_form() {

  $project_id = variable_get('gathercontent_project_id');
  $saved_items = variable_get('gathercontent_saved_items');
  if (is_array($saved_items) && isset($saved_items[$project_id])) {
    unset($saved_items[$project_id]);
    variable_set('gathercontent_saved_items', $saved_items);
  }

  $form['item_header'] = array(
    '#markup' => '<h2>' . check_plain(t('Success!')) . '</h2>',
  );
  $form['message'] = array(
    '#markup' => '<p>' . t('All of your selected content has been imported.') . '</p>',
  );
  $form['submit'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/content/gathercontent/items',
    '#title' => t('Go back to project'),
  );
  return $form;
}
