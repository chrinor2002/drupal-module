<?php
/**
 * @file
 * Contains item field mapping form.
 */

/**
 * Form constructor for mapping fields.
 */
function gathercontent_item_import_form($form, &$form_state) {
  gathercontent_check_step('item_import');
  $obj = gathercontent_get_obj();
  $obj->getStates();
  $obj->getItems(TRUE);
  $obj->getPostTypes();
  $obj->itemOverwriteDropdown();
  $obj->mapToDropdown();
  $obj->filtersDropdown();
  $obj->parentDropdown();
  $cur_settings = variable_get('gathercontent_saved_settings', '');
  variable_set('gathercontent_media_files', array());
  if (!is_array($cur_settings)) {
    $cur_settings = array();
  }
  $obj->data['saved_settings'] = $obj->val($cur_settings, variable_get('gathercontent_project_id'), array());

  drupal_add_js(array(
    'gathercontent' => array(
      'ajaxurl' => url('admin/config/content/gathercontent/import_item'),
      'media' => url('admin/config/content/gathercontent/media'),
      'finished' => url('admin/config/content/gathercontent/finished'),
      'error_message' => t('There was a problem connecting. Please try again later'),
      'hierarchical_post_types' => $obj->hierarchical,
    ),
  ), 'setting');

  $path = drupal_get_path('module', 'gathercontent');
  drupal_add_js($path . '/js/jquery-ui-1.10.3.custom.min.js');
  drupal_add_js($path . '/js/item_import.js');

  $form['modals'] = array(
    '#markup' => '
    <div class="gc_overlay"></div>
    <div class="gc_container gc_modal gc_importing_modal">
        <h2>' . t('Importing items and text content...') . '</h2>
        <label>' . t('Item:') . ' <span id="gc_item_title"></span><img src="' . file_create_url($path . '/images/ajax-loader-grey.gif') . '" alt="" /></label>
        <div id="current_item" class="progress">
            <div class="bar" style="width:0%"></div>
        </div>
    </div>
    <div class="gc_container gc_modal gc_repeating_modal">
        <h2>' . t('Repeating configuration...') . '</h2>
        <img src="' . file_create_url($path . '/images/ajax_loader_blue.gif') . '" alt="" />
    </div>'
  );

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
  $form['item_options']['select_items'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/content/gathercontent/items',
    '#title' => t('Select different items'),
    '#prefix' => '<div class="gc_search_items gc_cf"><div class="gc_left">',
    '#suffix' => '</div>',
  );
  $form['item_options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import selected items'),
    '#prefix' => '<div class="gc_right">',
    '#suffix' => '</div></div>',
  );

  $form['item_list'] = array();
  $form['items'] = array();
  $form['settings'] = array();
  $form = $obj->generateFormSettings($form, $obj->items, -1, TRUE);

  $form['item_options_footer'] = array();
  $form['item_options_footer']['select_items'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/content/gathercontent/items',
    '#title' => t('Select different items'),
    '#prefix' => '<div class="gc_search_items gc_search_bottom gc_cf"><div class="gc_left">',
    '#suffix' => '</div>',
  );
  $form['item_options_footer']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import selected items'),
    '#prefix' => '<div class="gc_right">',
    '#suffix' => '</div></div>',
  );

  return $form;
}

/**
 * Form theme handler for gathercontent_item_import_form().
 */
function theme_gathercontent_item_import_form($variables) {
  $obj = gathercontent_get_obj();

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
        array(
          'data' => drupal_render($form['items'][$id]),
          'class' => 'gc_checkbox',
        ),
      ),
      'no_striping' => TRUE,
    );
    $class = '';
    if (isset($form['settings'][$id]['fields'])) {
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
      'no_striping' => TRUE,
      'data-item-id' => $id,
      'data-parent-id' => $obj->getParentId($id),
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

  $output = drupal_render($form['modals']);
  $output .= drupal_render($form['item_header']);
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
 * Checks if a taxonomy term exists and adds it if needed.
 */
function _gathercontent_add_terms($vocab, $terms) {
  if (is_array($terms)) {
    $terms = array_filter($terms);
  }
  else {
    $terms = array_filter(explode(',', strip_tags($terms)));
  }
  $tags = array();
  foreach ($terms as $term) {
    $term = trim($term);
    if (!empty($term)) {
      if ($tax = taxonomy_get_term_by_name($term, $vocab)) {
        $tid = $tax[key($tax)]->tid;
        $tags[] = entity_metadata_wrapper('taxonomy_term', $tid)->value();
      }
      else {
        $tags[] = entity_property_values_create_entity('taxonomy_term', array(
          'name' => $term,
          'vocabulary' => $vocab,
        ))->save()->value();
      }
    }
  }
  return $tags;
}

/**
 * AJAX function to process individual items.
 */
function gathercontent_import_item() {
  $obj = gathercontent_get_obj();
  $out = array('error' => t('Verification failed, please refreshing the item and try again.'));
  if (isset($_POST['gc']) && isset($_POST['gc']['item_id'])) {
    $gc = $_POST['gc'];
    $item_id = $gc['item_id'];
    $obj->getPostTypes();
    $project_id = variable_get('gathercontent_project_id');
    $items = variable_get('gathercontent_saved_items');
    $file_counter = 0;
    $total_files = 0;
    $files = array(
      'files' => array(),
      'total_files' => 0,
    );

    if ($_POST['cur_counter'] == 0) {
      variable_set('gathercontent_media_files', array());
    }
    if (is_array($items) && isset($items[$project_id]) && isset($items[$project_id]['items'][$item_id])) {
      extract($items[$project_id]);
      $obj->getFiles($item_id);

      $item = $items[$item_id];

      $config = $obj->getFieldConfig($item, $obj->val($obj->files, $item_id, array()));

      $fields = $obj->val($gc, 'fields', array());

      $save_settings = array(
        'post_type' => $gc['post_type'],
        'overwrite' => $gc['overwrite'],
        'filter' => $gc['filter'],
        'parent_id' => $gc['parent_id'],
        'fields' => array(),
      );

      $create_new = TRUE;
      if ($save_settings['overwrite'] > 0) {
        $node = node_load($save_settings['overwrite']);
        if ($node !== FALSE) {
          $create_new = FALSE;
          $entity = entity_metadata_wrapper('node', $node);
        }
      }
      if ($create_new === TRUE) {
        $e = entity_create('node', array(
          'type' => $save_settings['post_type'],
        ));
        $e->uid = $GLOBALS['user']->uid;
        $entity = entity_metadata_wrapper('node', $e);
      }

      if ($save_settings['overwrite'] == 0) {
        $entity->title->set($item->name);
      }

      $field_values = array();
      $redirects = array();

      if (isset($gc['fields'])) {
        foreach ($gc['fields'] as $info) {
          $tab = $info['field_tab'];
          $map_to = $info['map_to'];
          $field_name = $info['field_name'];

          if (empty($map_to)) {
            continue;
          }

          if ($map_to == '_dont_import_') {
            $save_settings['fields'][$tab . '_' . $field_name] = $map_to;
            continue;
          }
          elseif (isset($config[$tab]) && isset($config[$tab]['elements'][$field_name])) {
            $field = $config[$tab]['elements'][$field_name];
          }
          else {
            continue;
          }

          $save_settings['fields'][$tab . '_' . $field_name] = $map_to;

          $sub_field = 'value';
          $regex = '/\[(.+)\]$/';
          if (preg_match($regex, $map_to, $matches)) {
            $sub_field = $matches[1];
            $map_to = preg_replace($regex, '', $map_to);
          }

          if ($map_to != 'title' && $map_to != 'metatags' && $map_to != 'redirect') {
            $cur_field = $entity->{$map_to}->info();
            if ($field['type'] == 'files') {
              if (is_array($field['value']) && count($field['value']) > 0) {
                $new_files = array();
                $dir = '';
                if (isset($cur_field['property info']['file'])) {
                  $info = field_info_instance('node', $map_to, $save_settings['post_type']);
                  if (isset($info['settings']['file_directory'])) {
                    $dir = $info['settings']['file_directory'];
                  }
                }
                foreach ($field['value'] as $file) {
                  $new_files[] = array(
                    'id' => $file->id,
                    'filename' => $file->filename,
                    'original_filename' => $file->original_filename,
                    'size' => $file->size,
                    'field' => $map_to,
                    'file_directory' => $dir,
                    'counter' => $file_counter,
                    'filter' => $save_settings['filter'],
                    'node_type' => $save_settings['post_type'],
                    'field_type' => $cur_field['type'],
                    'is_file_field' => isset($cur_field['property info']['file']),
                  );
                }

                $total_files += count($new_files);
                $files['files'][] = $new_files;
                $files['total_files'] = $total_files;

                $field['value'] = '#_gc_file_name_' . $file_counter . '#';
                $file_counter++;
              }
              else {
                $field['value'] = '';
              }
            }
          }

          if ($map_to === 'redirect') {
            foreach(array_map('trim', explode(',', strip_tags($field['value']))) as $redirect) {
              if (url_is_external($redirect)) {
                $redirect = parse_url($redirect, PHP_URL_PATH);
              }

              $uri = ltrim($redirect, '/');

              if (!valid_url($uri)) {
                continue;
              }

              $redirects[] = $uri;
            }
          }
          else if ($map_to === 'metatags') {
            $raw_entity = $entity->value();
            $raw_entity->metatags = array(LANGUAGE_NONE => array(
              'title' => $field,
            ));
          }
          else if ($map_to == 'title') {
            $entity->title->set(strip_tags($field['value']));
          }
          elseif (!empty($field['value'])) {
            if (isset($cur_field['property info']['file'])) {
              continue;
            }
            elseif (strpos($cur_field['type'], 'taxonomy_term') !== FALSE) {
              if (!is_array($field['value'])) {
                $field['value'] = array_filter(explode(',', strip_tags($field['value'])));
              }
              else {
                $field['value'] = array_filter($field['value']);
              }
              $is_list = (strpos($cur_field['type'], 'list') !== FALSE);
              if (count($field['value']) > 0) {
                if (isset($cur_field['options list'])) {
                  if ($is_list) {
                    $options = $entity->{$map_to}->optionsList();
                    $values = array();
                    foreach ($field['value'] as $term) {
                      $key = array_search($term, $options);
                      if ($key !== FALSE) {
                        $values[] = $key;
                      }
                      elseif (isset($options[$term])) {
                        $values[] = $term;
                      }
                      else {
                        $tag = entity_property_values_create_entity('taxonomy_term', array(
                          'name' => $term,
                          'vocabulary' => $cur_field['bundle'],
                        ))->save()->value();
                        $values[] = $tag->tid;
                      }
                    }
                    $entity->{$map_to}->set($values);
                  }
                  else {
                    $options = $entity->{$map_to}->optionsList();
                    $term = array_shift($field['value']);
                    $key = array_search($term, $options);
                    if ($key !== FALSE) {
                      $entity->{$map_to}->set($key);
                    }
                    elseif (isset($options[$term])) {
                      $entity->{$map_to}->set($term);
                    }
                    else {
                      $tag = entity_property_values_create_entity('taxonomy_term', array(
                        'name' => $term,
                        'vocabulary' => $cur_field['bundle'],
                      ))->save()->value();
                      $entity->{$map_to}->set($tag);
                    }
                  }
                }
                else {
                  $tags = _gathercontent_add_terms($cur_field['bundle'], $field['value']);
                  if (count($tags) > 0) {
                    if (method_exists($entity->{$map_to}, 'getBundle')) {
                      $entity->{$map_to}->set(array_shift($tags));
                    }
                    else {
                      $entity->{$map_to}->set($tags);
                    }
                  }
                }
              }
            }
            elseif (isset($cur_field['options list'])) {
              $options = $entity->{$map_to}->optionsList();
              if (!is_array($field['value'])) {
                $field['value'] = array(strip_tags($field['value']));
              }
              $values = array();
              foreach ($field['value'] as $val) {
                $key = array_search($val, $options);
                if ($key !== FALSE) {
                  $values[] = $key;
                }
                elseif (isset($options[$val])) {
                  $values[] = $val;
                }
              }
              if (count($values) > 0) {
                if (strpos($cur_field['type'], 'list') === 0) {
                  $entity->{$map_to}->set($values);
                }
                else {
                  $value = array_shift($values);
                  $entity->{$map_to}->set($value);
                }
              }
            }
            elseif ($cur_field['type'] == 'text_formatted') {
              if ($field['type'] == 'choice_checkbox') {
                if ($save_settings['filter'] == 'plain_text') {
                  $tmp = '';
                  foreach ($field['value'] as $value) {
                    $tmp .= $value . "\n";
                  }
                }
                else {
                  $tmp = '<ul>';
                  foreach ($field['value'] as $value) {
                    $tmp .= '<li>' . $value . '</li>';
                  }
                  $tmp .= "</ul>\n";
                  $field['value'] = $tmp;
                }
              }
              elseif ($field['type'] != 'text') {
                $field['value'] = '<p>' . $field['value'] . '</p>';
              }
              if ($save_settings['filter'] == 'plain_text') {
                $field['value'] = strip_tags($field['value']);
              }
              if (!isset($field_values[$map_to])) {
                $field_values[$map_to] = array(
                  $sub_field => '',
                  'format' => $save_settings['filter'],
                );
              }

              $val = $field_values[$map_to][$sub_field];
              if (!empty($val)) {
                $val .= "\n";
              }
              $val .= $field['value'];
              $field_values[$map_to][$sub_field] = $val;
            }
            else {
              $field['value'] = strip_tags($field['value']);
              $entity->{$map_to}->set($field['value']);
            }
          }
        }
      }
      foreach ($field_values as $name => $val) {
        $entity->{$name}->set($val);
      }

      $entity->save();
      $save_settings['overwrite'] = $entity->getIdentifier();

      $entity_uri = entity_uri($entity->type(), $entity->value());

      if (module_exists('redirect')) {
        foreach ($redirects as $uri) {
          $redirect = new stdClass();
          redirect_object_prepare($redirect);
          $redirect->source = $uri;
          $redirect->redirect = $entity_uri['path'];
          $redirect->language = LANGUAGE_NONE;
          // Check if the redirect exists before saving.
          $hash = redirect_hash($redirect);

          if (!redirect_load_by_hash($hash)) {
            redirect_save($redirect);
          }
        }
      }

      $mlid = gathercontent_create_menu_item($entity->title->value(), $save_settings['parent_id'], $entity_uri['path']);

      $media = variable_get('gathercontent_media_files');
      if (!isset($media['total_files'])) {
        $media['total_files'] = 0;
      }
      if ($total_files > 0) {
        $media[$save_settings['overwrite']] = $files;
        if (!isset($media['total_files'])) {
          $media['total_files'] = 0;
        }
        $media['total_files'] += $total_files;
        variable_set('gathercontent_media_files', $media);
      }

      $cur_settings = variable_get('gathercontent_saved_settings');
      if (!is_array($cur_settings)) {
        $cur_settings = array();
      }
      if (!isset($cur_settings[$project_id])) {
        $cur_settings[$project_id] = array();
      }

      $cur_settings[$project_id][$item_id] = $save_settings;
      variable_set('gathercontent_saved_settings', $cur_settings);

      $out = array(
        'success' => TRUE,
        'item_percent' => $obj->percent(++$_POST['cur_counter'], $_POST['total']),
        'redirect_url' => ($media['total_files'] > 0 ? 'media' : 'finished'),
        'item_id' => $item_id,
        'new_item_id'   => $mlid,
        'new_item_html' => '<li data-post-type="' . $save_settings['post_type'] . '"><a href="#" title="' . check_plain($entity->title->value()) . '" data-value="' . $mlid . '">' . check_plain($entity->title->value()) . '</a></li>',
      );
    }
    else {
      $out = array(
        'error' => t('There was a problem importing the item, please refresh and try again.'),
      );
    }
  }
  else {
    $out = array(
      'error' => t('There was a problem importing the item, please refresh and try again.'),
    );
  }
  echo json_encode($out);
  exit;
}

/**
 * Create a menu link for importing hierarchy
 */
function gathercontent_create_menu_item($link_title, $parent, $link_path) {
  if($parent != '0' && $parent != '_imported_item_') {
    $menu = explode(':', $parent);
    if(count($menu) > 1) {
      $link = array(
        'link_path' => $link_path,
        'link_title' => $link_title,
        'menu_name' => $menu[0],
        'parent' => $parent,
        'plid' => $menu[1],
      );
      $mlid = menu_link_save($link);
      if (!$mlid) {
        drupal_set_message(t('There was an error saving the menu link.'), 'error');
      }
      else {
        return $link['menu_name'].':'.$mlid;
      }
    }
  }
  return '';
}
