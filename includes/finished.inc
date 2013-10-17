<?php

function gathercontent_finished_form() {
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
