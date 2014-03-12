<?php
class GatherContent_Curl extends GatherContent_Functions {

  var $page_count = 0;
  var $page_ids = array();
  var $tbl_page_ids = array();
  var $api_error_shown = false;
  var $default_filter = '';
  var $ids_used = array();
  var $files = array();

  function get_field_config($obj,$files=array()){
    if($obj->config != '') {
      $config = json_decode(base64_decode($obj->config));
      $new_config = array();
      if($this->foreach_safe($config)) {

        foreach($config as $tab_pane) {

          $new_fields = array();

          if($this->foreach_safe($tab_pane->elements)) {
            foreach($tab_pane->elements as $element) {

              switch($element->type) {
                case 'text':
                  $val = $element->value;
                  if(!$element->plain_text) {
                    $val = preg_replace_callback('#\<p\>(.+?)\<\/p\>#s',
                      create_function(
                        '$matches',
                        'return "<p>".str_replace(array("\n","\r\n","\r"), " ", $matches[1])."</p>";'
                      ), $val);
                    $val = str_replace('</ul><',"</ul>\n<", $val);
                    $val = preg_replace('/\s*<\//m', '</', $val);
                    $val = preg_replace('/<\/p>\s*<p>/m', "</p>\n<p>", $val);
                    $val = preg_replace('/<\/p>\s*</m', "</p>\n<", $val);
                    $val = preg_replace('/<p>\s*<\/p>/m','<p>&nbsp;</p>',$val);
                    $val = str_replace(array('<ul><li','</li><li>', '</li></ul>'), array("<ul>\n\t<li","</li>\n\t<li>", "</li>\n</ul>"), $val);
                    $val = html_entity_decode($val);
                  }
                  break;

                case 'choice_radio':
                  $val = '';
                  foreach($element->options as $idx => $option) {
                    if($option->selected) {
                      if(isset($option->value)) {
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
                  foreach($element->options as $option) {
                    if($option->selected) {
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

          $new_config[strtolower($tab_pane->label)] = $new_fields;
        }
      }
      return $new_config;
    }
    return array();
  }

  function get_files($page_id){
    $files = $this->get('get_files_by_page',array('id'=>$page_id));
    if($files && isset($files->files) && $this->foreach_safe($files->files)){
      foreach($files->files as $file){
        if(!isset($this->files[$file->page_id]))
          $this->files[$file->page_id] = array();
        if(!isset($this->files[$file->page_id][$file->field]))
          $this->files[$file->page_id][$file->field] = array();
        $this->files[$file->page_id][$file->field][] = $file;
      }
    }
  }

  function get_projects(){
    $projects = $this->get('get_projects');

    $newprojects = array();
    if($projects && is_object($projects) && is_array($projects->projects)) {
      foreach($projects->projects as $project){
        $newprojects[$project->id] = array(
          'name' => $project->name,
          'page_count' => $project->page_count,
          'word' => ($project->page_count == 1 ? 'page' : 'pages'),
        );
      }
      asort($newprojects);
    }
    $this->data['projects'] = $newprojects;
  }

  function get_states(){
    $states = $this->get('get_custom_states_by_project',array('id'=>variable_get('gathercontent_project_id')));
    $new_states = array();
    if($states && $this->foreach_safe($states->custom_states)){
      foreach($states->custom_states as $state){
        $new_states[$state->id] = (object) array(
          'name' => $state->name,
          'color_id' => $state->color_id,
          'position' => $state->position
        );
      }
      @uasort($new_states,array(&$this,'sort_pages'));
    }
    $this->data['states'] = $new_states;
  }

  function get_projects_dropdown(){
    $html = '';
    $url = url('admin/config/content/gathercontent/set_project_id/');
    $project_id = variable_get('gathercontent_project_id');
    $title = '';
    if($this->foreach_safe($this->data['projects'])){
      foreach($this->data['projects'] as $id => $info){
        if($id == $project_id){
          $title = $info['name'];
        } else {
          $html .= '
          <li>
            <a href="'.$url.$id.'">'.$info['name'].'</a>
          </li>';
        }
      }
      if($html != ''){
        $html = $this->dropdown_html('<span>'.$title.'</span>',$html);
      }
    }
    $this->data['projects_dropdown'] = $html;
  }

  function get_state_dropdown(){
    $html = '
      <li>
        <a data-custom-state-name="All" href="#change-state"><span class="page-status"></span>  '.t('All').'</a>
      </li>';
    if($this->foreach_safe($this->data['states'])){
      foreach($this->data['states'] as $id => $state){
        $html .= '
        <li>
          <a data-custom-state-name="'.$state->name.'" data-custom-state-id="'.$id.'" href="#change-state"><span class="page-status page-state-color-'.$state->color_id.'"></span> '.$state->name.'</a>
        </li>';
      }
    }
    $this->data['state_dropdown'] = $this->dropdown_html('<i class="icon-filter"></i> <span>'.t('All').'</span>',$html);
  }

  function get_post_types(){
    $post_types = node_type_get_types();
    $html = '';
    $default = '';
    $new_post_types = array();
    foreach($post_types as $type => $info) {
      if($default == ''){
        $default = $type;
      }
      $html .= '
      <li>
        <a data-value="'.check_plain($type).'" href="#">'.$info->name.'</a>
      </li>';
      $new_post_types[$type] = $info->name;
    }
    $this->post_types = $new_post_types;
    $this->data['post_types_dropdown'] = $html;
    $this->default_post_type = $default;
  }

  function get_pages($save_pages=false){
    $pages = $this->get('get_pages_by_project',array('id'=>variable_get('gathercontent_project_id')));
    $original = array();
    $new_pages = array();
    $parent_array = array();
    $meta_pages = array();
    if($pages && is_array($pages->pages)){
      foreach($pages->pages as $page){
        $original[$page->id] = $page;
        $parent_id = $page->parent_id;
        if(!isset($parent_array[$parent_id])){
          $parent_array[$parent_id] = array();
        }
        $parent_array[$parent_id][$page->id] = $page;

        $this->page_count++;
      }
      foreach($parent_array as $parent_id => $page_array){
        $array = $page_array;
        @uasort($array,array(&$this,'sort_pages'));
        $parent_array[$parent_id] = $array;
      }
      if(isset($parent_array[0])){
        foreach($parent_array[0] as $id => $page){
          $new_pages[$id] = $page;
          $new_pages[$id]->children = $this->sort_recursive($parent_array,$id);
        }
      }
    }
    $this->pages = $new_pages;
    $this->original_array = $original;
    $this->meta_pages = $meta_pages;
    if($save_pages){
      $project_id = variable_get('gathercontent_project_id');
      $saved_pages = variable_get('gathercontent_saved_pages');
      if(!is_array($saved_pages)){
        $saved_pages = array();
      }
      $saved_pages[$project_id] = array('pages' => $original, 'meta' => $meta_pages);
      variable_set('gathercontent_saved_pages', $saved_pages);
    }
  }

  function page_overwrite_dropdown(){
    $html = '';

    foreach($this->post_types as $name => $title) {
      $nodes = db_select('node')
                ->fields('node', array('nid', 'title'))
                ->condition('type', $name, '=')
                ->execute()
                ->fetchAllKeyed();
      $this->page_ids[$name] = array();
      foreach($nodes as $id => $node) {
        $this->page_ids[$name][] = $id;
        if(empty($node)){
          $node = t('(no title)');
        }
        $title_text = $node;
        if(strlen($title_text) > 30){
          $title_text = substr($title_text,0,30).'...';
        }
        $html .= '
    <li data-post-type="'.$name.'"><a href="#" data-value="'.$id.'" title="'.check_plain($node).'">';
        $html .= $title_text.'</a></li>';
      }
    }
    if($html != ''){
      $html = '
      <li class="divider"></li>'.$html;
    }

    $html = '
      <li>
        <a href="#" data-value="0">'.t('New entry').'</a>
      </li>'.$html;
    $this->data['overwrite_select'] = $html;
  }

  function filters_dropdown() {
    $formatting = array();
    $html = '';
    $result = db_query("SELECT * FROM {filter_format}");
    foreach($result as $record) {
      if($this->default_filter == '') {
        $this->default_filter = $record->format;
      }
      $html .= '
      <li>
        <a href="#" data-value="'.$record->format.'">'.$record->name.'</a>
      </li>';
    }
    $this->data['formatting'] = $html;
  }

  function map_to_dropdown(){
    $html = '
      <li data-post-type="all" class="live_filter">
        <input type="text" class="live_filter" placeholder="'.check_plain(t('Search...')).'" />
      </li>';
    foreach($this->post_types as $name => $title) {
      $instances = field_info_instances('node', $name);
      if(isset($instances['body'])){
        unset($instances['body']);
        $html .= '
      <li data-post-type="'.$name.'" data-search="body">
        <a href="#" data-value="body">'.t('Body').'</a>
      </li>';
      }
      $html .= '
      <li data-post-type="'.$name.'" data-search="title">
        <a href="#" data-value="title">'.t('Title').'</a>
      </li>';
      foreach($instances as $field => $info) {
        $text = $info['label'];
        if(strlen($text) > 30){
          $text = substr($text,0,30).'...';
        }
        $html .= '
      <li data-post-type="'.$name.'" data-search="'.check_plain($info['label']).'">
        <a href="#" data-value="'.$field.'">'.check_plain($text).'</a>
      </li>';
      }
    }
    $html .= '
      <li class="divider" data-post-type="all"></li>
      <li data-post-type="all" data-search="'.t('Do Not Import').'">
        <a href="#" data-value="_dont_import_">'.t('Do Not Import').'</a>
      </li>';
    $this->data['map_to_select'] = $html;
  }

  function dropdown_html($val,$html,$input=false,$real_val=''){
    return '
        <div class="btn-group has_input">
            <a class="btn dropdown-toggle" data-toggle="dropdown" href="#">
                '.$val.'
                <span class="caret"></span>'.($input!==false?'<input type="hidden" name="'.$input.'" value="'.esc_attr($real_val).'" />':'').'
            </a>
            <ul class="dropdown-menu">
                '.$html.'
            </ul>
        </div>';
  }

  function generate_form_settings($form, $array, $index=-1,$show_settings=false) {
    $index++;
    $prefix = '';
    $selected = variable_get('gathercontent_selected_pages');
    if(!$this->foreach_safe($selected)){
      $selected = array();
    }
    if($index > 0) {
      for($i=0; $i<$index; $i++) {
        $prefix .= '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
      }
      $prefix .= '↳';
    }
    foreach($array as $id => $page){
      if($show_settings && !in_array($id, $selected)){
        if(isset($page->children) && count($page->children) > 0){
          $form = $this->generate_form_settings($form, $page->children, $index, $show_settings);
        }
        continue;
      }
      $form['page_list'][$id] = array(
        'page_state' => array(
          '#markup' => '<span data-page-state="'.$page->custom_state_id.'" class="page-status page-state-color-'.$this->data['states'][$page->custom_state_id]->color_id.'"></span>',
        ),
        'page_name' => array(
          '#markup' => $prefix.'<span>'.$page->name.'</span>',
          //'class' => 'page-name',
        )
      );
      $config = $this->get_field_config($page);
      $fields = $this->val($config, 'content', array());
      $meta = $this->val($config, 'meta', array());
      if($show_settings && (count($fields) == 0 && ($meta === false || count($meta) == 0))) {
        $form['pages'][$id] = array(
          '#markup' => '',
        );
      }
      else {
        $form['pages'][$id] = array(
          '#type' => 'checkbox',
          '#return_value' => $id,
          '#default_value' => ($show_settings?$id:0),
          '#parents' => array(
            'pages',
            $id,
          ),
        );
      }
      if($show_settings){
        $cur_settings = array();
        if(isset($this->data['saved_settings'][$id])){
          $cur_settings = $this->data['saved_settings'][$id];
        }
        if(count($fields) > 0 || ($meta !== false && count($meta) > 0)) {
          $form['settings'][$id] = array(
            '#prefix' => '<div>',
            '#suffix' => '</div>',
          );
          $form['settings'][$id]['settings_header'] = array(
            '#prefix' => '<div class="gc_settings_header gc_cf">',
            '#suffix' => '</div>',
            'import_as' => array(
              '#type' => 'hidden',
              '#prefix' => '<div class="gc_setting gc_import_as" id="gc_import_as_'.$id.'"><label>'.t('Import as ').'</label>'.$this->dropdown_html('<span></span>',$this->data['post_types_dropdown']),
              '#suffix' => '</div>',
              '#parents' => array(
                'post_type',
                $id,
              ),
              '#default_value' => $this->val($cur_settings,'post_type'),
            ),
            'import_to' => array(
              '#type' => 'hidden',
              '#prefix' => '<div class="gc_setting gc_import_to" id="gc_import_to_'.$id.'"><label>'.t('Import to ').'</label>'.$this->dropdown_html('<span></span>',$this->data['overwrite_select']),
              '#suffix' => '</div>',
              '#parents' => array(
                'overwrite',
                $id,
              ),
              '#default_value' => $this->val($cur_settings,'overwrite'),
            ),
            'filter' => array(
              '#type' => 'hidden',
              '#prefix' => '<div class="gc_setting gc_filter" id="gc_filter_'.$id.'"><label>'.t('Text format ').'</label>'.$this->dropdown_html('<span></span>',$this->data['formatting']),
              '#suffix' => '</div>',
              '#parents' => array(
                'filter',
                $id,
              ),
              '#default_value' => $this->val($cur_settings,'filter',$this->default_filter),
            ),
            'repeat' => array(
              '#markup' => '<div class="gc_setting repeat_config">
                    <label>'.t('Repeat this configuration').' <input type="checkbox" id="gc_repeat_'.$id.'" name="gc[repeat_'.$id.']" value="Y" /></label>
                  </div>',
            ),
          );
          $form['settings'][$id]['fields'] = array(
            '#prefix' => '<div class="gc_settings_fields" id="gc_fields_'.$id.'">',
            '#suffix' => '</div>',
          );
          $field_settings = $this->val($cur_settings,'fields',array());
          if(count($field_settings) > 0) {
            foreach($field_settings as $name => $value) {
              list($tab,$field_name) = explode('_', $name, 2);
              if($tab == 'content' && isset($fields[$field_name])) {
                $tmp = $this->field_settings($id, $fields[$field_name], $tab, '', $value);
                if($tmp != ''){
                  $form['settings'][$id]['fields'][] = $tmp;
                }
                unset($fields[$field_name]);
              }
              elseif($tab == 'meta' && $meta !== false && isset($meta[$field_name])) {
                $tmp = $this->field_settings($id, $meta[$field_name], $tab, ' (Meta)', $value);
                if($tmp != ''){
                  $form['settings'][$id]['fields'][] = $tmp;
                }
                unset($meta[$field_name]);
              }
            }
          }
          foreach($fields as $field) {
            $value = $this->val($field_settings, 'content_'.$field['name']);
            $tmp = $this->field_settings($id, $field, 'content', '', $value);
            if($tmp != ''){
              $form['settings'][$id]['fields'][] = $tmp;
            }
          }
          if($meta !== false) {
            foreach($meta as $field) {
              $value = $this->val($field_settings, 'meta_'.$field['name']);
              $tmp = $this->field_settings($id, $field, 'meta', ' (Meta)', $value);
              if($tmp != ''){
                $form['settings'][$id]['fields'][] = $tmp;
              }
            }
          }
        }
        else {
          $form['settings'][$id] = array(
            '#markup' => '
<div class="alert alert-info">'.
sprintf(t('This page is empty. You can %sadd some content to this page in GatherContent%s.')
  ,'<a href="https://'.variable_get('gathercontent_api_url').'.gathercontent.com/pages/view/'.variable_get('gathercontent_project_id').'/'.$id.'" target="_blank">',
    '</a>'
  ).
'</div>',
          );
        }
      }
      if(isset($page->children) && count($page->children) > 0){
        $form = $this->generate_form_settings($form, $page->children,$index);
      }
    }

    return $form;
  }

  function field_settings($id,$field,$tab='content',$name_suffix='',$val=''){
    if($field['type'] == 'section'){
      return '';
    }
    $fieldid = $id.'_'.md5($tab.'_'.$field['label']);
    $idcounter = 0;
    while(isset($this->ids_used[$fieldid])){
      $fieldid = $fieldid.$idcounter++;
    }
    $this->ids_used[$fieldid] = true;
    $arr = array(
      '#prefix' => '<div class="gc_settings_field gc_cf" data-field-tab="'.$tab.'" id="field_'.$fieldid.'"><div class="gc_move_field"></div>',
      '#suffix' => '</div>',
      'label' => array(
        '#markup' => '<div class="gc_field_name gc_left">'.check_plain($field['label']).$name_suffix.'</div>',
      ),
      'map_to' => array(
        '#type' => 'hidden',
        '#prefix' => '<div class="gc_field_map gc_right" id="gc_field_map_'.$fieldid.'"><span>'.t('Map to').'</span>'.$this->dropdown_html('<span></span>',$this->data['map_to_select']),
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

  function _curl($url,$curl_opts=array()){
    @set_time_limit(60);
    $session = curl_init();

    curl_setopt($session, CURLOPT_URL, $url);
    curl_setopt($session, CURLOPT_HEADER, false);
    //curl_setopt($session, CURLOPT_TIMEOUT, 50);
    curl_setopt($session, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($session, CURLOPT_SSL_VERIFYPEER, true);
    curl_setopt($session, CURLOPT_CAINFO, dirname(__FILE__).'/cacert.pem');

    curl_setopt_array($session, $curl_opts);

    $response = curl_exec($session);
    $httpcode = curl_getinfo($session, CURLINFO_HTTP_CODE);
    curl_close($session);
    return array('response' => $response, 'httpcode' => $httpcode);
  }

  function get($command = '', $postfields = array(), $api_url = '', $api_key='') {
    $api_url = $api_url == '' ? variable_get('gathercontent_api_url') : $api_url;
    $api_key = $api_key == '' ? variable_get('gathercontent_api_key') : $api_key;
    $api_url = 'https://'.$api_url.'.gathercontent.com/api/0.3/'.$command;
    $curl_opts = array(
      CURLOPT_HTTPAUTH => CURLAUTH_DIGEST,
      CURLOPT_HTTPHEADER => array('Accept: application/json', 'Content-Type: application/x-www-form-urlencoded'),
      CURLOPT_USERPWD => $api_key . ":x",
      CURLOPT_POST => true,
      CURLOPT_POSTFIELDS => http_build_query($postfields)
    );
    extract($this->_curl($api_url,$curl_opts));

    try {
      $resp = json_decode($response);

      if(isset($resp->success) && $resp->success === true){
        return $resp;
      } elseif(isset($resp->error)){
        $error = $resp->error;
        if($command == 'get_files_by_page' && $error == 'Files not found.') {
          return false;
        } else {
          if($error == 'You have to log in.'){
            if($this->api_error_shown){
              return false;
            }
            $error = $this->auth_error();
          }
          drupal_set_message(t($error), 'error');
        }
      } else {
        if(!$this->api_error_shown){
          drupal_set_message($this->auth_error(), 'error');
        }
      }
    } catch(Exception $e){
      drupal_set_message(t('There was a problem contacting the API. Please check your server allows it.'), 'error');
    }

    return false;
  }

  function auth_error(){
    $this->api_error_shown = true;
    return sprintf(t('There was a problem contacting the API. Please check your API credentials. %sAuth Settings%s'),'<a href="'.url('admin/config/content/gathercontent/login').'">','</a>');
  }

  function sort_recursive($pages,$current=0){
    $children = array();
    if(isset($pages[$current])){
      $children = $pages[$current];
      foreach($children as $id => $page){
        $children[$id]->children = $this->sort_recursive($pages,$id);
      }
    }
    return $children;
  }

  function sort_pages($a,$b){
    if($a->position == $b->position){
      if($a->id == $b->id){
        return 0;
      } else {
        return ($a->id < $b->id) ? -1 : 1;
      }
    }
    return ($a->position < $b->position) ? -1 : 1;
  }
}
