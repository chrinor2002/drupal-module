<?php
class GatherContent_Functions {

  var $data = array();
  var $formatting = array();

  function get_drupal_fields() {
    $fields = array();
    $result = db_query("SELECT con.data, con.type, con.field_name, con.id, ins.data as ins_data, ins.bundle FROM {field_config_instance} ins INNER JOIN {field_config} con ON ins.field_id = con.id");
    foreach($result as $record) {
      if(!isset($fields[$record->bundle])) {
        $fields[$record->bundle] = array();
      }

      $fields[$record->bundle][$record->field_name] = $this->_process_drupal_field($record);
    }

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
    return $fields;
  }

  function get_drupal_field($node_type,$field_name) {
    $result = db_query("SELECT con.data, con.type, con.field_name, con.id, ins.data as ins_data, ins.bundle FROM {field_config_instance} ins INNER JOIN {field_config} con ON ins.field_id = con.id WHERE ins.bundle=:node_type AND con.field_name=:field_name",
      array(
        'node_type' => $node_type,
        'field_name' => $field_name,
      ));
    if($result->rowCount() > 0) {
      return $this->_process_drupal_field($result->fetchObject());
    } else {
      return array(
        'type' => '',
        'formatting' => false,
        'allow_format_change' => false,
        'file_directory' => '',
      );
    }
  }

  function _process_drupal_field($record) {
    $arr = array('field_id' => $record->id, 'type' => $record->type);

    $tmp = @unserialize($record->data);

    if($arr['type'] == 'taxonomy_term_reference') {
      if(isset($tmp['settings']['allowed_values'][0])) {
        $arr['vocabulary'] = $tmp['settings']['allowed_values'][0]['vocabulary'];
      }
    }

    $has_formatting = false;
    if(isset($tmp['foreign keys']['format']['table']) &&
      $tmp['foreign keys']['format']['table'] == 'filter_format' &&
      isset($tmp['foreign keys']['format']['columns']['format'])) {
      $has_formatting = true;
    }
    $arr['formatting'] = $has_formatting;

    $allow_format_change = false;
    $tmp = @unserialize($record->ins_data);
    if($has_formatting) {
      if(isset($tmp['settings']['text_processing']) && $tmp['settings']['text_processing'] == 1) {
        $allow_format_change = true;
      }
    }
    if($arr['type'] == 'image' || $arr['type'] == 'file') {
      $dir = '';
      if(isset($tmp['settings']['file_directory'])) {
        $dir = $tmp['settings']['file_directory'];
      }
      $arr['file_directory'] = $dir;
    }
    $arr['allow_format_change'] = $allow_format_change;
    return $arr;
  }


  function get_page_title_array($post_id){
    $data = array();
    $post = node_load($post_id);
    $title = isset($post->title) ? $post->title : '';
    $title = empty($title) ? '(no title)' : $title;
    $data['original_title'] = check_plain(strip_tags($title));
    if(strlen($title) > 30){
      $title = substr($title,0,27).'...';
    }
    $data['page_title'] = $title;
    return $data;
  }

  function percent($num,$total){
    return number_format((($num / $total) * 100),2);
  }

  function foreach_safe($arr){
    if(is_array($arr) && count($arr) > 0){
      return true;
    }
    return false;
  }

  function val($array,$field,$default=''){
    if(is_array($array) && isset($array[$field])){
      return $array[$field];
    }
    return $default;
  }

  function add_media_to_content($post_id,$file,$field,$more_than_1=false){
    $image_types = array('image/gif', 'image/jpeg', 'image/png');
    $file_obj = $file['file_obj'];
    $image_file = in_array($file_obj->filemime, $image_types);
    $html = $file['url'];
    $node = node_load($post_id);
    $val = $node->{$file['field']}[$node->language];
    if($file['field_type'] == 'image' || $file['field_type'] == 'file') {
      $val[] = array(
        'fid' => $file_obj->fid,
        'display' => 1,
        'description' => '',
      );
    }
    else {

      $plain_text = true;
      $allow_format = false;
      if($file['filter'] != 'plain_text') {
        if($field['formatting']) {
          $plain_text = false;
        }
        if($field['allow_format_change']) {
          $plain_text = false;
          $allow_format = true;
        }
      } elseif($field['allow_format_change']) {
        $allow_format = true;
      }

      $tag = '#_gc_file_name_'.$file['counter'].'#';
      if(!$plain_text) {
        if($image_file){
          $html = '<a href="'.$file['url'].'"><img src="'.$file['url'].'" alt="'.check_plain($file_obj->filename).'" /></a>';
        }
        else {
          $html = '<a href="'.$file['url'].'">'.$file_obj->filename.'</a>';
        }
      }

      $html = "\n".$html."\n";

      if($more_than_1){
        $html .= $tag;
      }
      $val = array(array(
        'value' => str_replace($tag,$html,$val[0]['value']),
        'format' => $file['filter'],
      ));
    }
    $node->{$file['field']}[$node->language] = $val;
    $node = node_submit($node);
    node_save($node);
  }

  function get_media_ajax_output($post_id,$media,$cur_post,$page_total,$total){
    $cur_num = $_POST['cur_num'];
    $cur_total = $_POST['cur_total'];

    $next_id = $post_id;
    if($cur_num == $page_total){
      $page_percent = 100;
      $cur_num = 1;
      unset($media[$post_id]);
      $next_id = key($media);
    } else {
      $page_percent = $this->percent($cur_num,$page_total);
      $cur_num++;
      $media[$post_id] = $cur_post;
    }
    $media['total_files'] = $total;
    variable_set('gathercontent_media_files',$media);
    if($cur_total == $total){
      $next_id = $post_id;
      $page_percent = $overall_percent = '100';
    } else {
      $overall_percent = $this->percent($cur_total,$total);
    }
    $cur_total++;

    $data = $this->get_page_title_array($next_id);

    $out = array(
      'page_percent' => $page_percent,
      'overall_percent' => $overall_percent,
      'cur_num' => $cur_num,
      'cur_total' => $cur_total,
      'page_title' => $data['page_title'],
      'original_title' => $data['original_title'],
    );
    return $out;
  }
}
