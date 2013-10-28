<?php

function gathercontent_finished_form() {

  $project_id = variable_get('gathercontent_project_id');
  $saved_pages = variable_get('gathercontent_saved_pages');
  if(is_array($saved_pages) && isset($saved_pages[$project_id])){
    unset($saved_pages[$project_id]);
    variable_set('gathercontent_saved_pages', $saved_pages);
  }

  $form['page_header'] = array(
    '#markup' => '<h2>'.check_plain(t('Success!')).'</h2>',
  );
  $form['message'] = array(
    '#markup' => '<p>'.t('All of your selected content has been imported.').'</p>',
  );
  $form['submit'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/content/gathercontent/pages',
    '#title' => t('Go back to project'),
  );
  return $form;
}
