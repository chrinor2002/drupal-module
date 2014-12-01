<?php

function gathercontent_media_form($form, &$form_state) {
  gathercontent_check_step('media');

  drupal_add_js(array(
    'gathercontent' => array(
      'ajaxurl' => url('admin/config/content/gathercontent/download_media'),
      'redirecturl' => url('admin/config/content/gathercontent/finished'),
      'error_message' => t('There was a problem connecting. Please try again later'),
    ),
  ), 'setting');
  $path = drupal_get_path('module', 'gathercontent');
  drupal_add_js($path . '/js/media.js');

  $obj = gathercontent_get_obj();

  $media = variable_get('gathercontent_media_files');
  if(!(is_array($media) && isset($media['total_files']) && $media['total_files'] > 0)) {
    drupal_goto('admin/config/content/gathercontent/finished');
  }
  unset($media['total_files']);
  $post_id = key($media);
  extract($obj->get_page_title_array($post_id));


  $form['header'] = array(
    '#markup' => '<h2>'.t('Importing files').'</h2>',
  );

  $form['message'] = array(
    '#prefix' => '<div class="alert alert-success">',
    '#markup' => t('<strong>Heads up!</strong> This process can take a while, it depends on how many files you have attached to your pages. Just think how much time you\'re saving.'),
    '#suffix' => '</div>',
  );

  $form['current_page']['label'] = array(
    '#prefix' => '<label>',
    '#suffix' => '</label>',
    '#markup' => t('Page:').' <span id="gc_page_title" title="'.$original_title.'">'.$page_title.'</span><img src="'.file_create_url($path.'/images/ajax-loader-grey.gif').'" alt="" />',
  );

  $form['current_page']['progress_bar'] = array(
    '#markup' => '<div id="current_page" class="progress"><div class="bar" style="width:0%"></div></div>',
  );

  $form['overall']['label'] = array(
    '#prefix' => '<label>',
    '#suffix' => '</label>',
    '#markup' => t('Overall Progress'),
  );

  $form['overall']['progress_bar'] = array(
    '#markup' => '<div id="overall_files" class="progress"><div class="bar" style="width:0%"></div></div>',
  );

  $form['cancel_link'] = array(
    '#prefix' => '<div class="gc_center">',
    '#type' => 'link',
    '#href' => 'admin/config/content/gathercontent/pages_import',
    '#title' => t('Cancel'),
    '#suffix' => '</div>',
  );
  return $form;
}

function gathercontent_ajax_media_download() {
  $out = array('error' => t('Verification failed, please refreshing the page and try again.'));
  if(isset($_SERVER['HTTP_X_REQUESTED_WITH']) && $_SERVER['HTTP_X_REQUESTED_WITH'] == "XMLHttpRequest") {
    $obj = gathercontent_get_obj();
    $cur_num = $_POST['cur_num'];
    $cur_total = $_POST['cur_total'];
    $retry = $_POST['cur_retry'];

    $media = variable_get('gathercontent_media_files');
    $total = $media['total_files'];
    unset($media['total_files']);

    $post_id = key($media);
    if($obj->foreach_safe($media[$post_id]['files'])) {
      $cur_post = $media[$post_id];
      $page_total = $cur_post['total_files'];
      $more_than_1 = (count($cur_post['files'][0]) > 1);
      $file = array_shift($cur_post['files'][0]);
      $is_file_field = $file['is_file_field'];
      $field_type = $file['field_type'];
      $node = node_load($post_id);
      $entity = entity_metadata_wrapper('node', $node);

      if(!$more_than_1) {
        array_shift($cur_post['files']);
      }
      $file_obj = db_select('gathercontent_media', 'g')
                    ->fields('f',array('fid','filename','uri', 'filemime'))
                    ->condition('g.gid', $file['id']);
      $file_obj->join('file_managed', 'f', 'f.fid = g.fid');
      $file_obj = $file_obj->execute();
      if($file_obj->rowCount() > 0) {
        $file_obj = $file_obj->fetchObject();
        $obj->_add_media_to_entity($entity, $is_file_field, $file_obj, $file['field'], $more_than_1, $field_type, $file['counter']);

        $out = $obj->get_media_ajax_output($post_id,$media,$cur_post,$page_total,$total);
        $out['success'] = true;
      }
      else {
        $dir = 'public://';
        if(isset($field['file_directory'])) {
          $dir .= $field['file_directory'];
        }
        $filename = $file['original_filename'];
        if (substr($dir, -1) != '/') {
          $dir .= '/';
        }
        if(!file_prepare_directory($dir, FILE_CREATE_DIRECTORY)) {
          watchdog('file', 'The upload directory %directory for the file field !name could not be created or is not accessible. A newly uploaded file could not be saved in this directory as a consequence, and the upload was canceled.', array('%directory' => $dir, '!name' => $element['#field_name']));
        }
        $destination = file_destination($dir . $filename, FILE_EXISTS_RENAME);
        $resp = $obj->_curl('https://gathercontent.s3.amazonaws.com/'.$file['filename']);
        if($resp['httpcode'] == 200) {
          $fp = fopen($destination,'w');
          if(!$fp) {
            $real_destination = drupal_realpath($destination);
            $fp = fopen($real_destination,'w');
          }
          if($fp) {
            fwrite($fp,$resp['response']);
            fclose($fp);

            global $user;
            $new_file = (object) array(
              'fid' => NULL,
              'uri' => $destination,
              'filename' => drupal_basename($destination),
              'filemime' => file_get_mimetype($destination),
              'uid' => $user->uid,
              'status' => FILE_STATUS_PERMANENT,
            );
            $new_file = file_save($new_file);
            $gid = (object) array(
              'fid' => $new_file->fid,
              'gid' => $file['id'],
            );
            drupal_write_record('gathercontent_media', $gid);

            $obj->_add_media_to_entity($entity, $is_file_field, $new_file, $file['field'], $more_than_1, $field_type, $file['counter']);

            $out = $obj->get_media_ajax_output($post_id,$media,$cur_post,$page_total,$total);
            $out['success'] = true;
          }
          else {
            if($retry == '1'){
              $out = $obj->get_media_ajax_output($post_id,$media,$cur_post,$page_total,$total);
              $out['success'] = false;
              $out['error'] = sprintf(t('There was an error with the file (%s)'),$new_file);
            }
            else {
              $out = array(
                'success'=>false,
                'retry'=>true,
                'msg'=>sprintf(t('Retrying to download (%s)'),$file['original_filename'])
              );
            }
          }
        }
        else {
          if($retry == '1'){
            $out = $obj->get_media_ajax_output($post_id,$media,$cur_post,$page_total,$total);
            $out['success'] = false;
            $out['error'] = sprintf(t('There was an error with the file (%s)'),$new_file);
          }
          else {
            $out = array(
              'success'=>false,
              'retry'=>true,
              'msg'=>sprintf(t('Retrying to download (%s)'),$file['original_filename'])
            );
          }
        }
      }
    }
    else {
      if($retry == '1'){
        $out = $obj->get_media_ajax_output($post_id,$media,$cur_post,$page_total,$total);
        $out['success'] = false;
        $out['error'] = sprintf(t('Failed to download the file (%s)'),$file['original_filename']);
      }
      else {
        $out = array(
          'success'=>false,
          'retry'=>true,
          'msg'=>sprintf(t('Retrying to download (%s)'),$file['original_filename'])
        );
      }
    }
  }

  echo json_encode($out);
  exit;
}
