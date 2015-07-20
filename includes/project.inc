<?php
/**
 * @file
 * Contains project select form.
 */

/**
 * Form constructor for selecting project.
 */
function gathercontent_project_form($form, &$form_state) {
  gathercontent_check_step('projects');
  $obj = gathercontent_get_obj();
  $obj->getProjects();
  if (isset($obj->data['projects']) && count($obj->data['projects']) > 0) {
    $options = array();
    $default = NULL;
    foreach ($obj->data['projects'] as $id => $info) {
      if (is_null($default)) {
        $default = $id;
      }
      $options[$id] = $info['name'] . ' &mdash; <span class="item-count">' . $info['item_count'] . ' ' . $info['word'] . '</span>';
    }
    $form['header'] = array(
      '#prefix' => '<div class="gc_cf">',
      '#markup' => '<h2 class="gc_left">' . t('Choose a project to import content from') . '</h2>',
    );
    $form['link'] = array(
      '#type' => 'link',
      '#title' => t('Account settings'),
      '#href' => 'admin/config/content/gathercontent/login',
      '#prefix' => '<div class="gc_right">',
      '#suffix' => '</div></div>',
    );
    $form['gathercontent_project_id'] = array(
      '#type' => 'radios',
      '#default_value' => variable_get('gathercontent_project_id', $default),
      '#options' => $options,
      '#required' => TRUE,
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Import content'),
    );
  }
  else {
    $form['header'] = array(
      '#prefix' => '<div class="gc_cf">',
      '#markup' => '<h2 class="gc_left">' . t('No projects found') . '</h2>',
    );
    $form['link'] = array(
      '#type' => 'link',
      '#title' => t('Account settings'),
      '#href' => 'admin/config/content/gathercontent/login',
      '#prefix' => '<div class="gc_right">',
      '#suffix' => '</div></div>',
    );
    drupal_set_message(t('No projects found'), 'error');
  }
  return $form;
}

/**
 * Form submission handler for gathercontent_project_form().
 */
function gathercontent_project_form_submit($form, &$form_state) {
  variable_set('gathercontent_project_id', $form_state['values']['gathercontent_project_id']);
  drupal_goto('admin/config/content/gathercontent/items');
}

/**
 * Select project by ID and redirects to item selection.
 */
function gathercontent_set_project_id($project_id = 0) {
  variable_set('gathercontent_project_id', $project_id);
  drupal_goto('admin/config/content/gathercontent/items');
}
