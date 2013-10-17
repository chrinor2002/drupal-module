<?php

function gathercontent_pages_import_form($form, &$form_state) {
  gathercontent_check_step('pages_import');
  $obj = gathercontent_get_obj();
  $obj->get_states();
  $obj->get_pages(true);
  $obj->get_post_types();
  $obj->page_overwrite_dropdown();
  $obj->map_to_dropdown();
  $cur_settings = variable_get('gathercontent_saved_settings','');
  if(!is_array($cur_settings)) {
    $cur_settings = array();
  }
  $obj->data['saved_settings'] = $obj->val($cur_settings,variable_get('gathercontent_project_id'),array());

  $path = drupal_get_path('module', 'gathercontent');
  drupal_add_js($path . '/js/jquery-ui-1.10.3.custom.min.js');
  drupal_add_js($path . '/js/pages_import.js');

  $form['modal'] = array(
    '#markup' => '
    <div class="gc_overlay"></div>
    <div class="gc_container gc_modal">
        <h2>'.t('Importing pages and text content...').'</h2>
        <img src="'.url($path.'/images/ajax_loader_blue.gif').'" alt="" />
    </div>',
  );

  $form['page_header'] = array(
    '#prefix' => '<div class="gc_page_header gc_cf">',
    '#suffix' => '</div>',
  );
  $form['page_header']['title'] = array(
    '#markup' => '<h2>'.check_plain(t('Choose pages to import')).'</h2>',
  );
  $form['page_header']['account_link'] = array(
    '#type' => 'link',
    '#title' => t('Account settings'),
    '#href' => 'admin/config/content/gathercontent/login',
  );

  $form['page_options'] = array();
  $form['page_options']['select_pages'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/content/gathercontent/pages',
    '#title' => t('Select different pages'),
    '#prefix' => '<div class="gc_search_pages gc_cf"><div class="gc_left">',
    '#suffix' => '</div>',
  );
  $form['page_options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import selected pages'),
    '#prefix' => '<div class="gc_right">',
    '#suffix' => '</div></div>',
  );


  $form['page_list'] = array();
  $form['pages'] = array();
  $form['settings'] = array();
  $form = $obj->generate_form_settings($form,$obj->pages,-1,true);


  $form['page_options_footer'] = array();
  $form['page_options_footer']['select_pages'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/content/gathercontent/pages',
    '#title' => t('Select different pages'),
    '#prefix' => '<div class="gc_search_pages gc_search_bottom gc_cf"><div class="gc_left">',
    '#suffix' => '</div>',
  );
  $form['page_options_footer']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import selected pages'),
    '#prefix' => '<div class="gc_right">',
    '#suffix' => '</div></div>',
  );

  return $form;
}

function theme_gathercontent_pages_import_form($variables) {
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
      'no_striping' => true,
    );
    $class = '';
    if(isset($form['settings'][$id]['field_order'])) {
      $class = 'gc_settings_container';
    }
    $rows[] = array(
      'data' => array(
        array(
          'data' => drupal_render($form['settings'][$id]),
          'colspan' => 3,
          'class' => array($class),
        ),
      ),
      'no_striping' => true,
      'data-page-id' => $id,
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

  $output = drupal_render($form['modal']);
  $output .= drupal_render($form['page_header']);
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

function gathercontent_pages_import_form_submit($form, &$form_state) {
  $obj = gathercontent_get_obj();

  $gc = $form_state['values'];
  $chks = array('field_order','post_type','overwrite','filter');
  foreach($chks as $chk) {
    if(!isset($gc[$chk])) {
      $gc[$chk] = array();
    }
  }
  $fields = $obj->get_drupal_fields();
  $obj->get_post_types();
  $obj->get_pages(true);
  $obj->get_files();
  $new_pages = array();
  $files = array();
  $project_id = variable_get('gathercontent_project_id');
  if(isset($gc['pages'])) {
    $map_to = $obj->val($gc,'map_to',array());
    $field_tab = $obj->val($gc,'field_tab',array());
    $field_name = $obj->val($gc,'field_name',array());

    $field_values = array();
    $file_counter = 0;
    $total_file_counter = 0;

    $save_settings = array();

    foreach($gc['pages'] as $key => $page_id) {
      if($page_id > 0) {
        $cur_file_counter = 0;
        $cur_total_counter = 0;
        $cur_files = array('files' => array(), 'total_files' => 0);
        $page = $obj->original_array[$page_id];

        $field_order = explode(',', $obj->val($gc['field_order'],$key));

        $save_settings[$page_id] = array(
          'post_type' => $obj->val($gc['post_type'],$key),
          'overwrite' => $obj->val($gc['overwrite'],$key),
          'filter' => $obj->val($gc['filter'],$key),
          'fields' => array(),
        );
        $post_type = $save_settings[$page_id]['post_type'];

        $custom_fields = $obj->get_field_config($page,$obj->val($obj->files,$page_id,array()));
        $meta_id = 0;
        $meta_fields = array();
        if(isset($obj->meta_pages[$page_id])) {
          $meta_id = $obj->meta_pages[$page_id]->id;
          $meta_fields = $obj->get_field_config($obj->meta_pages[$page_id],$obj->val($obj->files,$meta_id,array()));
        }

        $node = new stdClass();
        $node->type = $save_settings[$page_id]['post_type'];
        $node->language = LANGUAGE_NONE;
        if($save_settings[$page_id]['overwrite'] > 0) {
          $node->nid = $save_settings[$page_id]['overwrite'];
        }
        node_object_prepare($node);
        $node->title = $page->name;
        if(isset($map_to[$page_id])){
          foreach($field_order as $idx) {
            if(!isset($map_to[$key][$idx])) {
              continue;
            }
            $fieldname = $map_to[$key][$idx];
            $tab = $field_tab[$key][$idx];
            $page_field = $field_name[$key][$idx];
            if($fieldname == '_dont_import_') {
              $save_settings[$page_id]['fields'][$tab.'_'.$page_field] = $fieldname;
              continue;
            }
            elseif($tab == 'meta') {
              $field = $meta_fields[$page_field];
            }
            elseif($tab == 'content') {
              $field = $custom_fields[$page_field];
            }
            else {
              continue;
            }
            $save_settings[$page_id]['fields'][$tab.'_'.$page_field] = $fieldname;
            if($fieldname != 'title') {
              $cur_fields = $fields[$post_type][$fieldname];
              if($field['type'] == 'attach') {
                if(is_array($field['value']) && count($field['value']) > 0) {
                  $new_files = array();
                  $dir = isset($cur_fields['file_directory']) ? $cur_fields['file_directory'] : '';
                  foreach($field['value'] as $file) {
                    $file = (array) $file;
                    $file['field'] = $fieldname;
                    $file['fid'] = $cur_fields['field_id'];
                    $file['file_directory'] = $dir;
                    $file['counter'] = $file_counter;
                    $file['filter'] = $save_settings[$page_id]['filter'];
                    $file['node_type'] = $save_settings[$page_id]['post_type'];
                    $new_files[] = $file;
                  }
                  $cur_total_counter += count($new_files);
                  $total_file_counter += count($new_files);
                  $cur_files['files'][] = $new_files;
                  $cur_files['total_files'] = $cur_total_counter;
                  //$post_ids[$post['ID']] = true;
                  $field['value'] = '#_gc_file_name_'.$file_counter.'#';
                  $file_counter++;
                  $cur_file_counter++;
                } else {
                  $field['value'] = '';
                }
              }
            }

            if($fieldname == 'title') {
              $node->title = $field['value'];
            }
            elseif(!empty($field['value'])) {
              if($cur_fields['type'] == 'file' || $cur_fields['type'] == 'image') {
                continue;
              }
              elseif($cur_fields['type'] == 'taxonomy_term_reference') {
                $tids = _gathercontent_add_terms($cur_fields['vocabulary'],$field['value']);
                foreach($tids as $tid) {
                  $node->{$fieldname}[$node->language][]['tid'] = $tid;
                }
              }
              else {
                $plain_text = true;
                $allow_format = false;
                if($save_settings[$page_id]['filter'] != 'plain_text') {
                  if($cur_fields['formatting']) {
                    $plain_text = false;
                  }
                  if($cur_fields['allow_format_change']) {
                    $plain_text = false;
                    $allow_format = true;
                  }
                } elseif($cur_fields['allow_format_change']) {
                  $allow_format = true;
                }

                if($field['type'] == 'checkbox' && is_array($field['value'])) {
                  $tmp = '<ul>';
                  foreach($field['value'] as $value) {
                    $tmp .= '<li>'.$value.'</li>';
                  }
                  $tmp .= "</ul>\n";
                  $field['value'] = $tmp;
                }
                elseif($field['type'] != 'paragraph') {
                  $field['value'] = '<p>'.$field['value'].'</p>';
                }
                if($plain_text) {
                  $field['value'] = strip_tags($field['value']);
                }
                if($allow_format) {
                  if(!isset($node->{$fieldname})) {
                    $node->{$fieldname}[$node->language][0] = array('value' => '', 'format' => $save_settings[$page_id]['filter']);
                  }
                  $val = $node->{$fieldname}[$node->language][0]['value'];
                  if($val != '') {
                    $val .= "\n";
                  }
                  $node->{$fieldname}[$node->language][0]['value'] = $val.$field['value'];
                } else {
                  if(!isset($node->{$fieldname})) {
                    $node->{$fieldname}[$node->language][0] = '';
                  }
                  $val = $node->{$fieldname}[$node->language][0];
                  if($val != '') {
                    $val .= "\n";
                  }
                  $node->{$fieldname}[$node->language][0] = $val.$field['value'];
                }
              }
            }
          }
        }
        $node = node_submit($node);
        node_save($node);
        $save_settings[$page_id]['overwrite'] = $node->nid;
        if($cur_files['total_files'] > 0) {
          $files[$node->nid] = $cur_files;
        }
      }
    }
    $files['total_files'] = $total_file_counter;
    $cur_settings = variable_get('gathercontent_saved_settings');
    if(!is_array($cur_settings)) {
      $cur_settings = array();
    }
    $cur_settings[$project_id] = $save_settings;
    variable_set('gathercontent_saved_settings',$cur_settings);
    variable_set('gathercontent_media_files',$files);
  }

  $step = 'finished';
  if(count($files) > 0) {
    $step = 'media';
  }
  drupal_goto('admin/config/content/gathercontent/'.$step);
}

function _gathercontent_add_terms($vocab, $terms) {
  if(is_array($terms)) {
    $terms = array_filter($terms);
  }
  else {
    $terms = array_filter(explode(',', strip_tags($terms)));
  }
  $tids = array();
  $vid = taxonomy_vocabulary_machine_name_load($vocab)->vid;
  if(count($terms) > 0) {
    foreach($terms as $term) {
      $term = trim($term);
      if($term != '') {
        if($tax = taxonomy_get_term_by_name($term, $vocab)) {
          $key = array_keys($tax);
          $tids[] = $key[0];
        }
        else {
          $new_term = array(
            'vid' => $vid,
            'name' => $term,
          );;
          $new_term = (object) $new_term;
          taxonomy_term_save($new_term);
          $tids[] = $new_term->tid;
        }
      }
    }
  }
  return $tids;
}
