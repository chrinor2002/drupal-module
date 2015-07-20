<?php
/**
 * @file
 * Class containing general functions used throughout the importing. Extended by GatherContentCurl.
 */

class GatherContentFunctions {

  public $data = array();
  public $formatting = array();
  public $error = '';

  /**
   * Get item title for media import item.
   */
  public function getItemTitleArray($post_id) {
    $data = array();
    $post = node_load($post_id);
    $title = isset($post->title) ? $post->title : '';
    $title = empty($title) ? '(no title)' : $title;
    $data['original_title'] = check_plain(strip_tags($title));
    if (strlen($title) > 30) {
      $title = substr($title, 0, 27) . '...';
    }
    $data['item_title'] = $title;
    return $data;
  }

  /**
   * Return hex color value for custom states.
   */
  public function customStateColor($color_id, $color_custom) {

    $colors = array(
      1 => '#C5C5C5',
      2 => '#FAA732',
      3 => '#5EB95E',
      4 => '#0E90D2',
      5 => '#ECD815',
      6 => '#DD4398',
      7 => '#954F99',
      9999 => $this->customColorHex($color_custom)
    );

    return $colors[$color_id];
  }

  /**
   * Return default hex color if none found.
   */
  private function customColorHex($color_custom) {

    if(empty($color_custom)) {
      $color_custom = '#999999';
    }

    return $color_custom;
  }

  /**
   * Return percentage of a total number.
   */
  public function percent($num, $total) {
    return number_format((($num / $total) * 100), 2);
  }

  /**
   * Checks if variable is an array and array length is more than 0.
   */
  public function foreachSafe($arr) {
    if (is_array($arr) && count($arr) > 0) {
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Check to see if key exists in array otherwise returns default param.
   */
  public function val($array, $field, $default = '') {
    if (is_array($array) && isset($array[$field])) {
      return $array[$field];
    }
    return $default;
  }

  /**
   * Used during media import to add downloaded media to node.
   */
  public function addMediaToEntity(&$entity, $is_file_field, $file_obj, $field, $more_than_1, $field_type, $counter) {
    if ($is_file_field) {
      $file_val = array('fid' => $file_obj->fid, 'display' => TRUE);
      if (strpos($field_type, 'list') === 0) {
        $cur_val = $entity->{$field}->value();
        if (!is_array($cur_val)) {
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
      if ($field_type == 'text_formatted') {
        $value['value'] = $this->addMediaToContent($value['value'], file_create_url($file_obj->uri), $file_obj->filename, $file_obj->filemime, $value['format'], $more_than_1, $counter);
        if (isset($value['safe_value'])) {
          unset($value['safe_value']);
        }
      }
      else {
        $value = $this->addMediaToContent($value, file_create_url($file_obj->uri), $file_obj->filename, $file_obj->filemime, 'plain_text', $more_than_1, $counter);
      }
      $entity->{$field}->set($value);
    }
    $entity->save();
  }

  /**
   * Called from addMediaToEntity to insert media item into content.
   */
  private function addMediaToContent($value, $file_url, $filename, $filemime, $filter, $more_than_1, $counter) {
    $image_types = array('image/gif', 'image/jpeg', 'image/png');
    $image_file = in_array($filemime, $image_types);
    $tag = '#_gc_file_name_' . $counter . '#';
    $html = $file_url;
    if ($filter != 'plain_text') {
      if ($image_file) {
        $html = '<a href="' . $file_url . '"><img src="' . $file_url . '" alt="' . check_plain($filename) . '" /></a>';
      }
      else {
        $html = '<a href="' . $file_url . '">' . $filename . '</a>';
      }
    }

    $html = "\n" . $html . "\n";
    if ($more_than_1) {
      $html .= $tag;
    }
    return str_replace($tag, $html, $value);
  }


  /**
   * Used during media import to return output array containing file details.
   */
  public function getMediaAjaxOutput($post_id, $media, $cur_post, $item_total, $total) {
    $cur_num = $_POST['cur_num'];
    $cur_total = $_POST['cur_total'];

    $next_id = $post_id;
    if ($cur_num == $item_total) {
      $item_percent = 100;
      $cur_num = 1;
      unset($media[$post_id]);
      $next_id = key($media);
    }
    else {
      $item_percent = $this->percent($cur_num, $item_total);
      $cur_num++;
      $media[$post_id] = $cur_post;
    }
    $media['total_files'] = $total;
    variable_set('gathercontent_media_files', $media);
    if ($cur_total == $total) {
      $next_id = $post_id;
      $item_percent = $overall_percent = '100';
    }
    else {
      $overall_percent = $this->percent($cur_total, $total);
    }
    $cur_total++;

    $data = $this->getItemTitleArray($next_id);

    if ($overall_percent == 100) {
      variable_set('gathercontent_media_files', array());
    }

    $out = array(
      'item_percent' => $item_percent,
      'overall_percent' => $overall_percent,
      'cur_num' => $cur_num,
      'cur_total' => $cur_total,
      'item_title' => $data['item_title'],
      'original_title' => $data['original_title'],
    );
    return $out;
  }
}
