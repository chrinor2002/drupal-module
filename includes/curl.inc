<?php
/**
 * @file
 * Contains functions used to process and retrieve data from GatherContent.
 */

class GatherContentCurl extends GatherContentFunctions {

  public $files = array();
  public $hierarchical = array();

  private $parentIds = array();
  private $apiErrorShown = FALSE;
  private $defaultFilter = '';
  private $idsUsed = array();
  private $hasTitles = array();

  /**
   * Return parent id associated with a specific item.
   */
  public function getParentId($item_id) {
    if (isset($this->parentIds[$item_id])) {
      return $this->parentIds[$item_id];
    }

    return 0;
  }

  /**
   * Process config field to return field array.
   */
  public function getFieldConfig($obj, $files = array()) {
    if ($obj->config != '') {
      $config = json_decode(base64_decode($obj->config));
      $new_config = array();
      $total_fields = 0;
      if ($this->foreachSafe($config)) {

        foreach ($config as $tab_pane) {

          $new_fields = array();

          if ($this->foreachSafe($tab_pane->elements)) {
            foreach ($tab_pane->elements as $element) {

              switch ($element->type) {
                case 'text':
                  $val = $element->value;
                  if (!$element->plain_text) {
                    $val = preg_replace_callback('#\<p\>(.+?)\<\/p\>#s',
                      create_function(
                        '$matches',
                        'return "<p>".str_replace(array("\n","\r\n","\r"), " ", $matches[1])."</p>";'
                      ), $val);
                    $val = str_replace('</ul><', "</ul>\n<", $val);
                    $val = preg_replace('/\s*<\//m', '</', $val);
                    $val = preg_replace('/<\/p>\s*<p>/m', "</p>\n<p>", $val);
                    $val = preg_replace('/<\/p>\s*</m', "</p>\n<", $val);
                    $val = preg_replace('/<p>\s*<\/p>/m', '<p>&nbsp;</p>', $val);
                    $val = str_replace(
                      array(
                        '<ul><li',
                        '</li><li>',
                        '</li></ul>'
                      ),
                      array(
                        "<ul>\n\t<li",
                        "</li>\n\t<li>",
                        "</li>\n</ul>"
                      ),
                      $val);
                    $val = html_entity_decode($val);
                    $val = trim(preg_replace('/\s+/', ' ', $val));
                    $val = preg_replace("/<p>\s/m", "<p>", $val);
                    $val = preg_replace('/<mark[^>]*>/i', '', $val);
                    $val = preg_replace('/<\/mark>/i', '', $val);
                  }
                  break;

                case 'choice_radio':
                  $val = '';
                  foreach ($element->options as $idx => $option) {
                    if ($option->selected) {
                      if (isset($option->value)) {
                        $val = $option->value;
                      }
                      else {
                        $val = $option->label;
                      }
                    }
                  }
                  break;

                case 'choice_checkbox':
                  $val = array();
                  foreach ($element->options as $option) {
                    if ($option->selected) {
                      $val[] = $option->label;
                    }
                  }
                  break;

                case 'files':
                  $val = $this->val($files, $element->name, array());
                  break;

                default:
                  continue 2;
                break;

              }
              $new_fields[$element->name] = array(
                'name' => $element->name,
                'label' => $element->label,
                'type' => $element->type,
                'value' => $val,
              );
            }
          }

          $total_fields += count($new_fields);

          $new_config[strtolower($tab_pane->name)] = array(
            'label' => $tab_pane->label,
            'elements' => $new_fields
          );
        }
      }

      $new_config['field_count'] = $total_fields;

      return $new_config;
    }
    return array();
  }

  /**
   * Get file array for a specific item.
   */
  public function getFiles($item_id) {
    $files = $this->get('get_files_by_page', array('id' => $item_id));
    if ($files && isset($files->files) && $this->foreachSafe($files->files)) {
      foreach ($files->files as $file) {
        if (!isset($this->files[$file->page_id])) {
          $this->files[$file->page_id] = array();
        }
        if (!isset($this->files[$file->page_id][$file->field])) {
          $this->files[$file->page_id][$file->field] = array();
        }
        $this->files[$file->page_id][$file->field][] = $file;
      }
    }
  }

  /**
   * Get project list.
   */
  public function getProjects() {
    $projects = $this->get('get_projects');

    $newprojects = array();
    if ($projects && is_object($projects) && is_array($projects->projects)) {
      foreach ($projects->projects as $project) {
        $newprojects[$project->id] = array(
          'name' => $project->name,
          'item_count' => $project->page_count,
          'word' => ($project->page_count == 1 ? 'item' : 'items'),
        );
      }
      asort($newprojects);
    }
    $this->data['projects'] = $newprojects;
  }

  /**
   * Get list of workflow states.
   */
  public function getStates() {
    $states = $this->get('get_custom_states_by_project', array('id' => variable_get('gathercontent_project_id')));
    $new_states = array();
    if ($states && $this->foreachSafe($states->custom_states)) {
      foreach ($states->custom_states as $state) {
        $new_states[$state->id] = (object) array(
          'name' => $state->name,
          'color' => $this->customStateColor($state->color_id, $state->color_custom),
          'position' => $state->position
        );
      }
      @uasort($new_states, array(&$this, 'sortItems'));
    }
    $this->data['states'] = $new_states;
  }

  /**
   * Get project dropdown.
   */
  public function getProjectsDropdown() {
    $html = '';
    $url = url('admin/config/content/gathercontent/set_project_id/');
    $project_id = variable_get('gathercontent_project_id');
    $title = '';
    if ($this->foreachSafe($this->data['projects'])) {
      foreach ($this->data['projects'] as $id => $info) {
        if ($id == $project_id) {
          $title = $info['name'];
        }
        else {
          $html .= '
          <li>
            <a href="' . $url . $id . '">' . $info['name'] . '</a>
          </li>';
        }
      }
      if ($html != '') {
        $html = $this->dropdownHtml('<span>' . $title . '</span>', $html);
      }
    }
    $this->data['projects_dropdown'] = $html;
  }

  /**
   * Get workflow state dropdown.
   */
  public function getStateDropdown() {
    $html = '
      <li>
        <a data-custom-state-name="All" href="#change-state"><span class="item-status"></span>  ' . t('All') . '</a>
      </li>';
    if ($this->foreachSafe($this->data['states'])) {
      foreach ($this->data['states'] as $id => $state) {
        $html .= '
        <li>
          <a data-custom-state-name="' . $state->name . '" data-custom-state-id="' . $id . '" href="#change-state"><span class="item-status item-state-color-9999" style="background-color:' . $state->color . '"></span> ' . $state->name . '</a>
        </li>';
      }
    }
    $this->data['state_dropdown'] = $this->dropdownHtml('<i class="icon-filter"></i> <span>' . t('All') . '</span>', $html);
  }

  /**
   * List post types.
   */
  public function getPostTypes() {
    $post_types = node_type_get_types();

    $html = '';

    $default = '';

    $new_post_types = array();

    foreach ($post_types as $type => $info) {
      if ($default == '') {
        $default = $type;
      }

      if(function_exists('menu_parent_options')) {
        $options = menu_parent_options(menu_get_menus(), $type, $type);
        if (!empty($options)) {
          $this->hierarchical[$type] = TRUE;
        }
      }

      if ($info->has_title == TRUE) {
        $this->hasTitles[$type] = $info->title_label;
      }

      $html .= '
      <li>
        <a data-value="' . check_plain($type) . '" href="#">' . $info->name . '</a>
      </li>';
      $new_post_types[$type] = $info->name;
    }
    $this->post_types = $new_post_types;
    $this->data['post_types_dropdown'] = $html;
    $this->default_post_type = $default;
  }

  /**
   * List items for the selected project.
   */
  public function getItems($save_items = FALSE) {
    $items = $this->get('get_pages_by_project', array('id' => variable_get('gathercontent_project_id')));
    $original = array();
    $new_items = array();
    $parent_array = array();
    $meta_items = array();
    if ($items && is_array($items->pages)) {
      foreach ($items->pages as $item) {
        $original[$item->id] = $item;
        $parent_id = $item->parent_id;
        if (!isset($parent_array[$parent_id])) {
          $parent_array[$parent_id] = array();
        }
        $parent_array[$parent_id][$item->id] = $item;
      }
      foreach ($parent_array as $parent_id => $item_array) {
        $array = $item_array;
        @uasort($array, array(&$this, 'sortItems'));
        $parent_array[$parent_id] = $array;
      }
      if (isset($parent_array[0])) {
        foreach ($parent_array[0] as $id => $item) {
          $new_items[$id] = $item;
          $new_items[$id]->children = $this->sortRecursive($parent_array, $id);
        }
      }
    }
    $this->items = $new_items;
    $this->original_array = $original;
    $this->meta_items = $meta_items;
    if ($save_items) {
      $project_id = variable_get('gathercontent_project_id');
      $saved_items = variable_get('gathercontent_saved_items');
      if (!is_array($saved_items)) {
        $saved_items = array();
      }
      $saved_items[$project_id] = array('items' => $original, 'meta' => $meta_items);
      variable_set('gathercontent_saved_items', $saved_items);
    }
  }

  /**
   * Return dropdown for item list to overwrite.
   */
  public function itemOverwriteDropdown() {
    $html = $this->getTreeList();
    if ($html != '') {
      $html = '
      <li class="divider"></li>' . $html;
    }

    $html = '
      <li>
        <a href="#" data-value="0">' . t('New entry') . '</a>
      </li>' . $html;
    $this->data['overwrite_select'] = $html;
  }

  /**
   * Return dropdown for parent item list.
   */
  public function parentDropdown() {
    $html = '<li>
      <a href="#" data-value="0">' . t('No Menu Link') . '</a>
    </li>
    <li class="imported-item">
      <a href="#" data-value="_imported_item_">' . t('Parent Being Imported') . '</a>
    </li>';
    if(function_exists('menu_parent_options')) {
      $html .= '
    <li class="divider"></li>';

      $types = array_keys($this->hierarchical);
      foreach ($types as $type) {
        $options = menu_parent_options(menu_get_menus(), $type, $type);

        foreach ($options as $id => $item_name) {
          $html .= '
      <li data-post-type="' . $type . '">
        <a href="#" data-value="' . $id . '">' . check_plain($item_name) . '</a>
      </li>';
        }
      }
    }

    $this->data['parent_select'] = $html;
  }

  /**
   * Return list of items.
   */
  private function getTreeList() {
    if (isset($this->data['tree_list'])) {
      return $this->data['tree_list'];
    }

    $html = '';

    foreach ($this->post_types as $name => $title) {
      $nodes = db_select('node')
                ->fields('node', array('nid', 'title'))
                ->condition('type', $name, '=')
                ->execute()
                ->fetchAllKeyed();
      foreach ($nodes as $id => $node) {
        if (empty($node)) {
          $node = t('(no title)');
        }
        $title_text = $node;
        if (strlen($title_text) > 30) {
          $title_text = substr($title_text, 0, 30) . '...';
        }
        $html .= '
    <li data-post-type="' . $name . '"><a href="#" data-value="' . $id . '" title="' . check_plain($node) . '">';
        $html .= $title_text . '</a></li>';
      }
    }

    $this->data['tree_list'] = $html;

    return $this->data['tree_list'];
  }

  /**
   * Load text filter dropdowns.
   */
  public function filtersDropdown() {
    $formatting = array();
    $html = '';
    $result = db_query("SELECT * FROM {filter_format}");
    foreach ($result as $record) {
      if ($this->defaultFilter == '') {
        $this->defaultFilter = $record->format;
      }
      $html .= '
      <li>
        <a href="#" data-value="' . $record->format . '">' . $record->name . '</a>
      </li>';
    }
    $this->data['formatting'] = $html;
  }

  /**
   * Generate dropdown for entity fields.
   */
  public function mapToDropdown() {
    $html = '
      <li data-post-type="all" class="live_filter">
        <input type="text" class="live_filter" placeholder="' . check_plain(t('Search...')) . '" />
      </li>';
    foreach ($this->post_types as $name => $title) {
      $instances = field_info_instances('node', $name);

      if (module_exists('metatag') && function_exists('metatag_entity_supports_metatags')) {
        if (metatag_entity_supports_metatags('node', $name)) {
          $instances['metatags'] = array(
            'label' => t('Metatags'),
          );
        }
      }

      if (module_exists('redirect')) {
        $instances['redirect'] = array(
          'label' => t('Redirect'),
        );
      }

      if (isset($instances['body'])) {
        unset($instances['body']);
        $html .= '
      <li data-post-type="' . $name . '" data-search="body">
        <a href="#" data-value="body">' . t('Body') . '</a>
      </li>';

        $html .= '
      <li data-post-type="' . $name . '" data-search="summary">
        <a href="#" data-value="body[summary]">' . t('Summary') . '</a>
      </li>';
      }

      if (isset($this->hasTitles[$name])) {
        $html .= '
      <li data-post-type="' . $name . '" data-search="' . check_plain($this->hasTitles[$name]) . '">
        <a href="#" data-value="title">' . $this->hasTitles[$name] . '</a>
      </li>';
      }
      foreach ($instances as $field => $info) {
        $text = $info['label'];
        if (strlen($text) > 30) {
          $text = substr($text, 0, 30) . '...';
        }
        $html .= '
      <li data-post-type="' . $name . '" data-search="' . check_plain($info['label']) . '">
        <a href="#" data-value="' . $field . '">' . check_plain($text) . '</a>
      </li>';
      }
    }
    $html .= '
      <li class="divider" data-post-type="all"></li>
      <li data-post-type="all" data-search="' . t('Do Not Import') . '">
        <a href="#" data-value="_dont_import_">' . t('Do Not Import') . '</a>
      </li>';
    $this->data['map_to_select'] = $html;
  }

  /**
   * Generate dropdown html for a given object.
   */
  private function dropdownHtml($val, $html, $input = FALSE, $real_val = '') {
    return '
        <div class="btn-group has_input">
            <a class="btn dropdown-toggle" data-toggle="dropdown" href="#">
                ' . $val . '
                <span class="caret"></span>' . ($input !== FALSE ? '<input type="hidden" name="' . $input . '" value="' . esc_attr($real_val) . '" />' : '') . '
            </a>
            <ul class="dropdown-menu">
                ' . $html . '
            </ul>
        </div>';
  }

  /**
   * Return option to be selected in the parent dropdown.
   */
  private function getParentItemId($parent_id, $selected_items, $cur_settings) {
    $new_parent_id = 0;
    if (isset($cur_settings['parent_id'])) {
      $new_parent_id = $cur_settings['parent_id'];
    }
    else {
      if ($parent_id > 0) {
        if (in_array($parent_id, $selected_items)) {
          $new_parent_id = '_imported_item_';
        }
        else {
          $cur_settings = variable_get('gathercontent_saved_settings', array());
          $project_id = variable_get('gathercontent_project_id');
          if (isset($cur_settings[$project_id]) &&
            isset($cur_settings[$project_id][$parent_id])) {
            $new_parent_id = $cur_settings[$project_id][$parent_id]['overwrite'];
          }
        }
      }
    }

    return $new_parent_id;
  }

  /**
   * Generate item settings for item_import item.
   */
  public function generateFormSettings($form, $array, $index = -1, $show_settings = FALSE) {
    $index++;
    $prefix = '';
    $selected = variable_get('gathercontent_selected_items');
    if (!$this->foreachSafe($selected)) {
      $selected = array();
    }
    if ($index > 0) {
      for ($i = 0; $i < $index; $i++) {
        $prefix .= '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
      }
      $prefix .= '↳';
    }
    foreach ($array as $id => $item) {
      if ($show_settings && !in_array($id, $selected)) {
        if (isset($item->children) && count($item->children) > 0) {
          $form = $this->generateFormSettings($form, $item->children, $index, $show_settings);
        }
        continue;
      }
      $form['item_list'][$id] = array(
        'item_state' => array(
          '#markup' => '<span data-item-state="' . $item->custom_state_id . '" class="item-status item-state-color-9999" style="background-color:' . $this->data['states'][$item->custom_state_id]->color . '"></span>',
        ),
        'item_name' => array(
          '#markup' => $prefix . '<span>' . $item->name . '</span>',
        )
      );

      $config = $this->getFieldConfig($item);

      $field_count = $this->val($config, 'field_count', 0);

      if($show_settings && $field_count == 0) {
        $form['items'][$id] = array(
          '#markup' => '',
        );
      }
      else {
        $form['items'][$id] = array(
          '#type' => 'checkbox',
          '#return_value' => $id,
          '#default_value' => ($show_settings ? $id : 0),
          '#parents' => array(
            'items',
            $id,
          ),
        );
      }
      if ($show_settings) {
        $cur_settings = array();
        if (isset($this->data['saved_settings'][$id])) {
          $cur_settings = $this->data['saved_settings'][$id];
        }
        if ($field_count > 0) {
          $parent_id = $item->parent_id;
          $this->parentIds[$item->id] = $item->parent_id;
          $parent_id_value = $this->getParentItemId($parent_id, $selected, $cur_settings);
          $form['settings'][$id] = array(
            '#prefix' => '<div>',
            '#suffix' => '</div>',
          );
          $form['settings'][$id]['settings_header'] = array(
            '#prefix' => '<div class="gc_settings_header gc_cf" data-parent-id="' . $parent_id . '">',
            '#suffix' => '</div>',
            'gc_cf' => array(
              '#prefix' => '<div class="gc_cf">',
              '#suffix' => '</div>',
              'import_as' => array(
                '#type' => 'hidden',
                '#prefix' => '<div class="gc_setting gc_import_as" id="gc_import_as_' . $id . '"><label>' . t('Import as ') . '</label>' . $this->dropdownHtml('<span></span>', $this->data['post_types_dropdown']),
                '#suffix' => '</div>',
                '#parents' => array(
                  'post_type',
                  $id,
                ),
                '#default_value' => $this->val($cur_settings, 'post_type'),
              ),
              'import_to' => array(
                '#type' => 'hidden',
                '#prefix' => '<div class="gc_setting gc_import_to" id="gc_import_to_' . $id . '"><label>' . t('Import to ') . '</label>' . $this->dropdownHtml('<span></span>', $this->data['overwrite_select']),
                '#suffix' => '</div>',
                '#parents' => array(
                  'overwrite',
                  $id,
                ),
                '#default_value' => $this->val($cur_settings, 'overwrite'),
              ),
              'filter' => array(
                '#type' => 'hidden',
                '#prefix' => '<div class="gc_setting gc_filter" id="gc_filter_' . $id . '"><label>' . t('Text format ') . '</label>' . $this->dropdownHtml('<span></span>', $this->data['formatting']),
                '#suffix' => '</div>',
                '#parents' => array(
                  'filter',
                  $id,
                ),
                '#default_value' => $this->val($cur_settings, 'filter', $this->defaultFilter),
              ),
              'parent' => array(
                '#type' => 'hidden',
                '#prefix' => '<div class="gc_setting gc_parent" id="gc_parent_' . $id . '"><label>' . t('Parent ') . '</label>' . $this->dropdownHtml('<span></span>', $this->data['parent_select']),
                '#suffix' => '</div>',
                '#parents' => array(
                  'parent',
                  $id,
                ),
                '#default_value' => $parent_id_value,
              ),
            ),
            'repeat' => array(
              '#markup' => '<div class="gc_setting repeat_config"><label>' . t('Repeat this configuration') . ' <input type="checkbox" id="gc_repeat_' . $id . '" name="gc[repeat_' . $id . ']" value="Y" /></label></div>',
            ),
          );
          $form['settings'][$id]['fields'] = array(
            '#prefix' => '<div class="gc_settings_fields" id="gc_fields_' . $id . '">',
            '#suffix' => '</div>',
          );
          $field_settings = $this->val($cur_settings, 'fields', array());
          if (count($field_settings) > 0) {
            foreach ($field_settings as $name => $value) {
              list($tab,$field_name) = explode('_', $name, 2);

              if(isset($config[$tab]) && isset($config[$tab]['elements'][$field_name])) {
                $tmp = $this->fieldSettings($id, $config[$tab]['elements'][$field_name], $tab, $config[$tab]['label'], $value);
                if($tmp != '') {
                  $form['settings'][$id]['fields'][] = $tmp;
                }
                unset($config[$tab]['elements'][$field_name]);
              }
            }
          }
          unset($config['field_count']);
          foreach($config as $tab_name => $tab) {
            foreach($tab['elements'] as $field) {
              $value = $this->val($field_settings, $tab_name.'_'.$field['name']);
              $tmp = $this->fieldSettings($id, $field, $tab_name, $tab['label'], $value);
              if($tmp != '') {
                $form['settings'][$id]['fields'][] = $tmp;
              }
            }
          }
        }
        else {
          $form['settings'][$id] = array(
            '#markup' => '<div class="alert alert-info">' . sprintf(t('This item is empty. You can %sadd some content to this item in GatherContent%s.'), '<a href="https://' . variable_get('gathercontent_api_url') . '.gathercontent.com/page/' . $id . '" target="_blank">', '</a>') . '</div>',
          );
        }
      }
      if (isset($item->children) && count($item->children) > 0) {
        $form = $this->generateFormSettings($form, $item->children, $index, $show_settings);
      }
    }

    return $form;
  }

  /**
   * Generate individual field options.
   */
  private function fieldSettings($id, $field, $tab = 'content', $tab_label = '', $val = '') {
    if ($field['type'] == 'section') {
      return '';
    }
    $fieldid = $id . '_' . md5($tab . '_' . $field['label']);
    $idcounter = 0;
    while (isset($this->idsUsed[$fieldid])) {
      $fieldid = $fieldid . $idcounter++;
    }
    $this->idsUsed[$fieldid] = TRUE;
    $arr = array(
      '#prefix' => '<div class="gc_settings_field gc_cf" data-field-tab="' . $tab . '" id="field_' . $fieldid . '"><div class="gc_move_field"></div>',
      '#suffix' => '</div>',
      'label' => array(
        '#markup' => '<div class="gc_field_name gc_left"><div class="gc_tab_name gc_tooltip" title="' . t('Tab') . '">' . check_plain($tab_label) . '</div>' . check_plain($field['label']) . '</div>',
      ),
      'map_to' => array(
        '#type' => 'hidden',
        '#prefix' => '<div class="gc_field_map gc_right" id="gc_field_map_' . $fieldid . '"><span>' . t('Map to') . '</span>' . $this->dropdownHtml('<span></span>', $this->data['map_to_select']),
        '#suffix' => '</div>',
        '#parents' => array(
          'map_to',
          $id,
          '',
        ),
        '#default_value' => $val
      ),
      'field_tab' => array(
        '#type' => 'hidden',
        '#default_value' => $tab,
        '#parents' => array(
          'field_tab',
          $id,
          '',
        )
      ),
      'field_name' => array(
        '#type' => 'hidden',
        '#default_value' => $field['name'],
        '#parents' => array(
          'field_name',
          $id,
          '',
        )
      ),
    );
    return $arr;
  }

  /**
   * Used to retrieve data from GatherContent and to download files.
   */
  public function curl($url, $curl_opts = array()) {
    @set_time_limit(60);
    $session = curl_init();

    curl_setopt($session, CURLOPT_URL, $url);
    curl_setopt($session, CURLOPT_HEADER, FALSE);
    curl_setopt($session, CURLOPT_RETURNTRANSFER, TRUE);
    curl_setopt($session, CURLOPT_SSL_VERIFYPEER, TRUE);
    curl_setopt($session, CURLOPT_CAINFO, dirname(__FILE__) . '/cacert.pem');

    curl_setopt_array($session, $curl_opts);

    $response = curl_exec($session);
    $httpcode = curl_getinfo($session, CURLINFO_HTTP_CODE);
    curl_close($session);
    return array('response' => $response, 'httpcode' => $httpcode);
  }

  /**
   * Call GatherContent API function.
   */
  public function get($command = '', $postfields = array(), $api_url = '', $api_key = '') {
    $api_url = $api_url == '' ? variable_get('gathercontent_api_url') : $api_url;
    $api_key = $api_key == '' ? variable_get('gathercontent_api_key') : $api_key;
    $api_url = 'https://' . $api_url . '.gathercontent.com/api/0.4/' . $command;
    $curl_opts = array(
      CURLOPT_HTTPAUTH => CURLAUTH_DIGEST,
      CURLOPT_HTTPHEADER => array('Accept: application/json', 'Content-Type: application/x-www-form-urlencoded'),
      CURLOPT_USERPWD => $api_key . ":x",
      CURLOPT_POST => TRUE,
      CURLOPT_POSTFIELDS => http_build_query($postfields)
    );
    extract($this->curl($api_url, $curl_opts));

    try {
      $resp = json_decode($response);

      if (isset($resp->success) && $resp->success === TRUE) {
        return $resp;
      }
      elseif (isset($resp->error)) {
        $error = $resp->error;
        if ($command == 'get_files_by_page' && $error == 'Files not found.') {
          return FALSE;
        }
        else {
          if ($error == 'You have to log in.') {
            if ($this->apiErrorShown) {
              return FALSE;
            }
            $error = $this->authError();
          }
          drupal_set_message(t($error), 'error');
        }
      }
      else {
        if (!$this->apiErrorShown) {
          drupal_set_message($this->authError(), 'error');
        }
      }
    }
    catch(Exception $e){
      drupal_set_message(t('There was a problem contacting the API. Please check your server allows it.'), 'error');
    }

    return FALSE;
  }

  /**
   * Return GatherContent API authentication error.
   */
  private function authError() {
    $this->apiErrorShown = TRUE;
    return sprintf(t('There was a problem contacting the API. Please check your API credentials. %sAuth Settings%s'), '<a href="' . url('admin/config/content/gathercontent/login') . '">', '</a>');
  }

  /**
   * Sort items recursively.
   */
  private function sortRecursive($items, $current = 0) {
    $children = array();
    if (isset($items[$current])) {
      $children = $items[$current];
      foreach ($children as $id => $item) {
        $children[$id]->children = $this->sortRecursive($items, $id);
      }
    }
    return $children;
  }

  /**
   * Sort items by position and id.
   */
  private function sortItems($a, $b) {
    if ($a->position == $b->position) {
      if ($a->id == $b->id) {
        return 0;
      }
      else {
        return ($a->id < $b->id) ? -1 : 1;
      }
    }
    return ($a->position < $b->position) ? -1 : 1;
  }
}
