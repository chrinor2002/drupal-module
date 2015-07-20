<?php
/**
 * @file
 * Item for importing media via AJAX.
 */

/**
 * Form constructor for displaying media import dialog.
 */
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
  if (!(is_array($media) && isset($media['total_files']) && $media['total_files'] > 0)) {
    drupal_goto('admin/config/content/gathercontent/finished');
  }
  unset($media['total_files']);
  $post_id = key($media);
  extract($obj->getItemTitleArray($post_id));

  $form['header'] = array(
    '#markup' => '<h2>' . t('Importing files') . '</h2>',
  );

  $form['message'] = array(
    '#prefix' => '<div class="alert alert-success">',
    '#markup' => t('<strong>Heads up!</strong> This process can take a while, it depends on how many files you have attached to your items. Just think how much time you\'re saving.'),
    '#suffix' => '</div>',
  );

  $form['current_item']['label'] = array(
    '#prefix' => '<label>',
    '#suffix' => '</label>',
    '#markup' => t('Item:') . ' <span id="gc_item_title" title="' . $original_title . '">' . $item_title . '</span><img src="' . file_create_url($path . '/images/ajax-loader-grey.gif') . '" alt="" />',
  );

  $form['current_item']['progress_bar'] = array(
    '#markup' => '<div id="current_item" class="progress"><div class="bar" style="width:0%"></div></div>',
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
    '#href' => 'admin/config/content/gathercontent/item_import',
    '#title' => t('Cancel'),
    '#suffix' => '</div>',
  );
  return $form;
}

/**
 * AJAX function to process individual media item.
 */
function gathercontent_ajax_media_download() {
  $out = array('error' => t('Verification failed, please refreshing the item and try again.'));
  if (isset($_SERVER['HTTP_X_REQUESTED_WITH']) && $_SERVER['HTTP_X_REQUESTED_WITH'] == "XMLHttpRequest") {
    $obj = gathercontent_get_obj();
    $cur_num = $_POST['cur_num'];
    $cur_total = $_POST['cur_total'];
    $retry = $_POST['cur_retry'];

    $media = variable_get('gathercontent_media_files');
    $total = $media['total_files'];
    unset($media['total_files']);

    $post_id = key($media);
    if(is_array($media[$post_id]['files']) && count($media[$post_id]['files']) > 0) {
      $cur_post = $media[$post_id];
      $item_total = $cur_post['total_files'];
      $more_than_1 = (count($cur_post['files'][0]) > 1);
      $file = array_shift($cur_post['files'][0]);
      $is_file_field = $file['is_file_field'];
      $field_type = $file['field_type'];
      $node = node_load($post_id);
      $entity = entity_metadata_wrapper('node', $node);

      if (!$more_than_1) {
        array_shift($cur_post['files']);
      }
      $file_obj = db_select('gathercontent_media', 'g')
                    ->fields('f', array('fid', 'filename', 'uri', 'filemime'))
                    ->condition('g.gid', $file['id']);
      $file_obj->join('file_managed', 'f', 'f.fid = g.fid');
      $file_obj = $file_obj->execute();
      if ($file_obj->rowCount() > 0) {
        $file_obj = $file_obj->fetchObject();
        $obj->addMediaToEntity($entity, $is_file_field, $file_obj, $file['field'], $more_than_1, $field_type, $file['counter']);

        $out = $obj->getMediaAjaxOutput($post_id, $media, $cur_post, $item_total, $total);
        $out['success'] = TRUE;
      }
      else {
        $dir = 'public://';
        if (isset($field['file_directory'])) {
          $dir .= $field['file_directory'];
        }
        $filename = $file['original_filename'];
        if (substr($dir, -1) != '/') {
          $dir .= '/';
        }
        if (!file_prepare_directory($dir, FILE_CREATE_DIRECTORY)) {
          watchdog('file', 'The upload directory %directory for the file field !name could not be created or is not accessible. A newly uploaded file could not be saved in this directory as a consequence, and the upload was canceled.', array('%directory' => $dir, '!name' => $element['#field_name']));
        }
        $destination = file_destination($dir . $filename, FILE_EXISTS_RENAME);
        $resp = $obj->curl('https://gathercontent.s3.amazonaws.com/' . $file['filename']);
        if ($resp['httpcode'] == 200) {
          $fp = fopen($destination, 'w');
          if (!$fp) {
            $real_destination = drupal_realpath($destination);
            $fp = fopen($real_destination, 'w');
          }
          if ($fp) {
            fwrite($fp, $resp['response']);
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

            $obj->addMediaToEntity($entity, $is_file_field, $new_file, $file['field'], $more_than_1, $field_type, $file['counter']);

            $out = $obj->getMediaAjaxOutput($post_id, $media, $cur_post, $item_total, $total);
            $out['success'] = TRUE;
          }
          else {
            if ($retry == '1') {
              $out = $obj->getMediaAjaxOutput($post_id, $media, $cur_post, $item_total, $total);
              $out['success'] = FALSE;
              $out['error'] = sprintf(t('There was an error with the file (%s)'), $new_file);
            }
            else {
              $out = array(
                'success' => FALSE,
                'retry' => TRUE,
                'msg' => sprintf(t('Retrying to download (%s)'), $file['original_filename'])
              );
            }
          }
        }
        else {
          if ($retry == '1') {
            $out = $obj->getMediaAjaxOutput($post_id, $media, $cur_post, $item_total, $total);
            $out['success'] = FALSE;
            $out['error'] = sprintf(t('There was an error with the file (%s)'), $new_file);
          }
          else {
            $out = array(
              'success' => FALSE,
              'retry' => TRUE,
              'msg' => sprintf(t('Retrying to download (%s)'), $file['original_filename'])
            );
          }
        }
      }
    }
    else {
      if ($retry == '1') {
        $out = $obj->getMediaAjaxOutput($post_id, $media, $cur_post, $item_total, $total);
        $out['success'] = FALSE;
        $out['error'] = sprintf(t('Failed to download the file (%s)'), $file['original_filename']);
      }
      else {
        $out = array(
          'success' => FALSE,
          'retry' => TRUE,
          'msg' => sprintf(t('Retrying to download (%s)'), $file['original_filename'])
        );
      }
    }
  }

  echo json_encode($out);
  exit;
}
