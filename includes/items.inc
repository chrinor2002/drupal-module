<?php
/**
 * @file
 * Contains selecting items to process.
 */

/**
 * Form constructor for selecting items.
 */
function gathercontent_item_form($form, &$form_state) {
  gathercontent_check_step('items');
  $obj = gathercontent_get_obj();
  $obj->getProjects();
  $obj->getStates();
  $obj->getItems();
  $obj->getStateDropdown();
  $obj->getProjectsDropdown();

  $form['item_header'] = array(
    '#prefix' => '<div class="gc_item_header gc_cf">',
    '#suffix' => '</div>',
  );
  $form['item_header']['title'] = array(
    '#markup' => '<h2>' . check_plain(t('Choose items to import')) . '</h2>',
  );
  $form['item_header']['account_link'] = array(
    '#type' => 'link',
    '#title' => t('Account settings'),
    '#href' => 'admin/config/content/gathercontent/login',
  );

  $form['item_options'] = array();
  $form['item_options']['project_dropdown'] = array(
    '#prefix' => '<div class="gc_search_items gc_cf"><div class="gc_left">',
    '#markup' => $obj->data['projects_dropdown'],
    '#suffix' => '</div>',
  );
  $form['item_options']['live_filter'] = array(
    '#prefix' => '<div class="gc_right">',
    '#suffix' => '</div></div>',
  );
  $form['item_options']['live_filter']['state_dropdown'] = array(
    '#markup' => $obj->data['state_dropdown'],
  );
  $form['item_options']['live_filter']['filter_input'] = array(
    '#markup' => '<input type="text" name="search" id="gc_live_filter" placeholder="' . check_plain(t('Search...')) . '" />',
  );
  $form['item_options']['live_filter']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Configure selected items'),
  );

  $form['item_list'] = array();
  $form['items'] = array();
  $form = $obj->generateFormSettings($form, $obj->items);

  $form['item_options_bottom'] = array(
    '#prefix' => '<div class="gc_search_items gc_search_bottom gc_cf"><div class="gc_right">',
    '#type' => 'submit',
    '#value' => t('Configure selected items'),
    '#suffix' => '</div></div>',
  );

  return $form;
}

/**
 * Form theme handler for gathercontent_items_form().
 */
function theme_gathercontent_item_form($variables) {
  $form = $variables['form'];
  $rows = array();

  foreach (element_children($form['item_list']) as $id) {

    $rows[] = array(
      'data' => array(
        drupal_render($form['item_list'][$id]['item_state']),
        array(
          'data' => drupal_render($form['item_list'][$id]['item_name']),
          'class' => 'item-name',
        ),
        drupal_render($form['items'][$id]),
      ),
    );
  }

  $header = array(
    array(
      'data' => '',
      'class' => 'item-state',
    ),
    array(
      'data' => t('Items'),
      'class' => 'item-name',
    ),
    '<input type="checkbox" class="gc_toggle_all" />'
  );

  $output = drupal_render($form['item_header']);
  $output .= drupal_render($form['item_options']);
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'gc_itemlist'),
    'empty' => t('No items available.')
  ));
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Form submission handler for gathercontent_items_form().
 */
function gathercontent_item_form_submit($form, &$form_state) {
  $selected_items = array();
  if (isset($form_state['values']['items'])) {
    foreach ($form_state['values']['items'] as $id => $val) {
      if ($val > 0) {
        $selected_items[] = $id;
      }
    }
  }
  if (count($selected_items) > 0) {
    variable_set('gathercontent_selected_items', $selected_items);
    drupal_goto('admin/config/content/gathercontent/item_import');
  }
  else {
    drupal_set_message(t('No items selected'), 'error');
  }
}
