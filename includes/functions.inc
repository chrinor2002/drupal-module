<?php
class GatherContent_Functions {

  var $data = array();
  var $formatting = array();

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

  function _add_media_to_entity(&$entity, $is_file_field, $file_obj, $field, $more_than_1, $field_type, $counter) {
    if($is_file_field) {
      $file_val = array('fid' => $file_obj->fid, 'display' => TRUE);
      if(strpos($field_type, 'list') === 0) {
        $cur_val = $entity->{$field}->value();
        if(!is_array($cur_val)) {
          $cur_val = array();
        }
        $cur_val[] = $file_val;
        $entity->{$field}->set($cur_val);
      }
      else {
        $entity->{$field}->set($file_val);
      }
    }
    else {
      $value = $entity->{$field}->value();
      if($field_type == 'text_formatted') {
        $value['value'] = $this->_add_media_to_content($value['value'], file_create_url($file_obj->uri), $file_obj->filename, $file_obj->filemime, $value['format'], $more_than_1, $counter);
        if(isset($value['safe_value'])) {
          unset($value['safe_value']);
        }
      }
      else {
        $value = $this->_add_media_to_content($value, file_create_url($file_obj->uri), $file_obj->filename, $file_obj->filemime, 'plain_text', $more_than_1, $counter);
      }
      $entity->{$field}->set($value);
    }
    $entity->save();
  }

  function _add_media_to_content($value, $file_url, $filename, $filemime, $filter, $more_than_1, $counter) {
    $image_types = array('image/gif', 'image/jpeg', 'image/png');
    $image_file = in_array($filemime, $image_types);
    $tag = '#_gc_file_name_'.$counter.'#';
    $html = $file_url;
    if($filter != 'plain_text') {
      if($image_file) {
        $html = '<a href="'.$file_url.'"><img src="'.$file_url.'" alt="'.check_plain($filename).'" /></a>';
      }
      else {
        $html = '<a href="'.$file_url.'">'.$filename.'</a>';
      }
    }

    $html = "\n".$html."\n";
    if($more_than_1) {
      $html .= $tag;
    }
    return str_replace($tag, $html, $value);
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
