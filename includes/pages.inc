<?php
/**
 * @file
 * Contains selecting pages to process.
 */

/**
 * Form constructor for selecting pages.
 */
function gathercontent_pages_form($form, &$form_state) {
  gathercontent_check_step('pages');
  $obj = gathercontent_get_obj();
  $obj->getProjects();
  $obj->getStates();
  $obj->getPages();
  $obj->getStateDropdown();
  $obj->getLevelDropdown();
  $obj->getProjectsDropdown();

  $form['page_header'] = array(
    '#prefix' => '<div class="gc_page_header gc_cf">',
    '#suffix' => '</div>',
  );
  $form['page_header']['title'] = array(
    '#markup' => '<h2>' . check_plain(t('Choose pages to import')) . '</h2>',
  );
  $form['page_header']['account_link'] = array(
    '#type' => 'link',
    '#title' => t('Account settings'),
    '#href' => 'admin/config/content/gathercontent/login',
  );

  $form['page_options'] = array();
  $form['page_options']['project_dropdown'] = array(
    '#prefix' => '<div class="gc_search_pages gc_cf"><div class="gc_left">',
    '#markup' => $obj->data['projects_dropdown'],
    '#suffix' => '</div>',
  );
  $form['page_options']['live_filter'] = array(
    '#prefix' => '<div class="gc_right">',
    '#suffix' => '</div></div>',
  );
  $form['page_options']['live_filter']['state_dropdown'] = array(
    '#markup' => $obj->data['state_dropdown'],
  );
  $form['page_options']['live_filter']['level_dropdown'] = array(
    '#markup' => $obj->data['level_dropdown'],
  );
  $form['page_options']['live_filter']['filter_input'] = array(
    '#markup' => '<input type="text" name="search" id="gc_live_filter" placeholder="' . check_plain(t('Search...')) . '" />',
  );
  $form['page_options']['live_filter']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Configure selected pages'),
  );

  $form['page_list'] = array();
  $form['pages'] = array();
  $form = $obj->generateFormSettings($form, $obj->pages);

  $form['page_options_bottom'] = array(
    '#prefix' => '<div class="gc_search_pages gc_search_bottom gc_cf"><div class="gc_right">',
    '#type' => 'submit',
    '#value' => t('Configure selected pages'),
    '#suffix' => '</div></div>',
  );

  return $form;
}

/**
 * Form theme handler for gathercontent_pages_form().
 */
function theme_gathercontent_pages_form($variables) {
  $form = $variables['form'];
  $rows = array();

  foreach (element_children($form['page_list']) as $id) {

    $rows[] = array(
      'data' => array(
        drupal_render($form['page_list'][$id]['page_state']),
        array(
          'data' => drupal_render($form['page_list'][$id]['page_name']),
          'class' => 'page-name',
        ),
        drupal_render($form['pages'][$id]),
      ),
    );
  }

  $header = array(
    array(
      'data' => '',
      'class' => 'page-state',
    ),
    array(
      'data' => t('Pages'),
      'class' => 'page-name',
    ),
    '<input type="checkbox" class="gc_toggle_all" />'
  );

  $output = drupal_render($form['page_header']);
  $output .= drupal_render($form['page_options']);
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'gc_pagelist'),
    'empty' => t('No pages available.')
  ));
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Form submission handler for gathercontent_pages_form().
 */
function gathercontent_pages_form_submit($form, &$form_state) {
  $selected_pages = array();
  if (isset($form_state['values']['pages'])) {
    foreach ($form_state['values']['pages'] as $id => $val) {
      if ($val > 0) {
        $selected_pages[] = $id;
      }
    }
  }
  if (count($selected_pages) > 0) {
    variable_set('gathercontent_selected_pages', $selected_pages);
    drupal_goto('admin/config/content/gathercontent/pages_import');
  }
  else {
    drupal_set_message(t('No pages selected'), 'error');
  }
}
