<?php
// $Id$

/**
 * @file
 * Dynamic Banner Admin Pages and various other functions to make them work
 * Most of the code in this file was derived from path module
 */

/**
 * Return a listing of all defined URL aliases.
 * When filter key passed, perform a standard search on the given key,
 * and return the list of matching URL aliases.
 */
function dynamic_banner_admin_page() {
  $output = ""; // default 

  // grab the filter if the user set one
  $filter = dynamic_banner_build_filter_query();
  
  // Add the filter form above the overview table.
  //$output['dynamic_banner_admin_filter_form'] = drupal_get_form('dynamic_banner_admin_filter_form');
  
  // construct the headers of the table
  $header = array(
    array(
      'data' => t('Url'),
      'field' => 'd.path',
      'sort' => 'asc',
    ),
    array(
      'data' => t('ImgUrl'),
      'field' => 'd.imgurl',
    ),
    array(
      'data' => t('Text'),
      'field' => 'd.text',
    ),
    array(
      'data' => t('Link'),
      'field' => 'd.link',
    ),
    array(
      'data' => t('Operations'),
      'colspan' => '2',
    ),
  );
  

  $query = db_select('dynamic_banner', 'd')->extend('PagerDefault')->extend('TableSort'); // alias the table name to d
  $query->fields('d', array('dbid', 'path', 'imgurl', 'text', 'link', 'mode', 'start_time', 'end_time')) // Select the specified fields from the node table. 
    ->limit(20) // Load the first 20 rows.
    ->orderByHeader($header);
    
  if (!empty($filter['where'])) {
    $query->where($filter['where'], $filter['args']);
  }
  $result = $query->execute();

  dpm($result);

  // start constructing the individual rows
  $rows = array();
  foreach ($result as $data) {
    if ( $data->dbid == 999) {
      $rows[] = array('data' => 
        array('DEFAULT',
        $data->imgurl,
        $data->text,
        $data->link,
        l(t('edit'), "admin/structure/banners/default"),
        l(t('delete'), "admin/structure/banners/delete/" . $data->dbid)));
    }
    else {
      $rows[] = array('data' => 
        array($data->path,
        $data->imgurl,
        $data->text,
        $data->link,
        l(t('edit'), "admin/structure/banners/edit/$data->dbid"),
        l(t('delete'), "admin/structure/banners/delete/" . $data->dbid)));
    }
  }
  
  $output['dynamic_banner_table'] = array(
    '#theme' => 'table', 
    '#header' => $header, 
    '#rows' => $rows, 
    //'#attributes' => array('id' => 'admin-dblog'), 
    '#empty' => t('No Banners Found.'),
  );
  $output['dynamic_banner_pager'] = array('#theme' => 'pager');

  // let drupal handle print and echo
  return $output;
}

/**
 * Menu callback; handles pages for creating and editing banners
 */
function dynamic_banner_admin_edit($dbid = 0) {
  if ($dbid) {
    $banner = dynamic_banner_load($dbid);
    drupal_set_title($banner['path']);
    $output = drupal_get_form('dynamic_banner_admin_form', $banner);
  }
  else {
    $output = drupal_get_form('dynamic_banner_admin_form');
  }

  return $output;
}

/**
 * Needed for the filtering of the banners page
 */
function dynamic_banner_build_filter_query() {
  if (empty($_SESSION['dynamic_banner_filter'])) {
    return;
  }

  $filters = dynamic_banner_filters();

  // Build query
  $where = $args = array();
  foreach ($_SESSION['dynamic_banner_filter'] as $key => $filter) {
    $filter_where = array();
    foreach ($filter as $value) {
      $filter_where[] = $filters[$key]['where'];
      $args[] = $value;
    }
    if (!empty($filter_where)) {
      $where[] = '(' . implode(' OR ', $filter_where) . ')';
    }
  }
  $where = !empty($where) ? implode(' AND ', $where) : '';

  return array(
    'where' => $where, 
    'args' => $args,
  );
}

/**
 * The specific filters that can be used
 */
function dynamic_banner_filters() {
  $filters = array();
  $filters['type'] = array(
    'title' => t('Type'),
    'where' => 'd.url ?',
    'options' => array('NOT LIKE %* AND NOT LIKE %!','LIKE %*','LIKE %!'));
  return $filters;
}

/**
 * Menu callback; handles pages for creating and editing the default banner (999)
 */
function dynamic_banner_default_admin_edit($dbid = 999) {
  if ($dbid) {
    $banner = dynamic_banner_load($dbid);
    drupal_set_title($banner['path']);
    $output = drupal_get_form('dynamic_banner_default_admin_form', $banner);
  }
  else {
    $output = drupal_get_form('dynamic_banner_default_admin_form');
  }

  return $output;
}

/**
 *
 */
function dynamic_banner_default_admin_form($form, &$form_state, $edit = array('path' => '', 'imgurl' => '', 'text' => '', 'link' => '')) {
  $form['#banner'] = $edit;

  /********* TODO MAKE LOGIC FOR IMAGE UPLOADER AND CHOOSER!!! *************/
  $form['imgurl'] = array(
    '#type' => 'textfield',
    '#title' => t('Image Url'),
    '#default_value' => $edit['imgurl'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the path to the image. Starting from the root site directory(comma seperated for randoms).'),
    '#field_prefix' => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q='),
    //'#required' => TRUE,
  );

  // banner picture chooser
  $form['picture'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default Banner'),
  );

  $picture = $edit['imgurl'];
  //if we are editing an existing pictue place that code in here
  if ($picture) {
    $form['picture']['current_picture'] = array('#value' => "<img src='/" . $picture . "'/>");
    $form['picture']['picture_delete'] = array(
      '#type' => 'checkbox',
      '#title' => t('Delete Banner'),
      '#description' => t('Check this box to delete your current picture.'),
    );
  }
  else {
    $form['picture']['picture_delete'] = array('#type' => 'hidden');
  }

  /*
   $form['picture']['picture_upload'] = array(
   '#type' => 'file',
   '#title' => t('Upload Banner'),
   '#size' => 64,
   //'#description' => t('Maximum dimensions are %dimensions and the maximum size is %size kB.', array('%dimensions' => '800x800', '%size' => '1024')),
   );*/

  $form['new']['attach'] = array(
    '#type' => 'submit',
    // this changes the submit box
    //'#value' => t('Attach Image'),
    '#name' => 'attach',
    '#ahah' => array(
      'path' => 'upload/js',
      'wrapper' => 'attach-wrapper',
      'progress' => array(
        'type' => 'bar',
        'message' => t('Please wait...'),
      ),
    ),
    '#submit' => array('node_form_submit_build_node'),
  );
  $form['#validate'][] = 'dynamic_banner_validate_picture';

  $form['text'] = array(
    '#type' => 'textfield',
    '#title' => t('Text'),
    '#default_value' => $edit['text'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the text to associate with this banner (comma seperated for randoms) (also must match amount of elements from images) -optional.'),
    '#required' => FALSE,
  );

  $form['link'] = array(
    '#type' => 'textfield',
    '#title' => t('Link'),
    '#default_value' => $edit['link'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the link you want your banner to point to (optional).'),
    '#field_prefix' => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q='),
    '#required' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => f('Update Banner'),
  );

  return $form;
}

/**
 * validate a banner to actually be a picture
 */
function dynamic_banner_validate_picture(&$form, &$form_state) {
  // Validate the uploaded picture.
  $validators = array(
    'file_validate_is_image' => array(),
  );

  if ($file = file_save_upload('picture_upload', $validators)) {
    // Remove the old picture.
    //if (isset($form_state['values']['_account']->picture) && file_exists($form_state['values']['_account']->picture)) {
    //file_delete($form_state['values']['_account']->picture);
    //}

    // The image was saved using file_save_upload() and was added to the
    // files table as a temporary file. We'll make a copy and let the garbage
    // collector delete the original upload.
    $info = image_get_info($file->filepath);
    $destination = "/sites/all/themes/banners/" . $file->filename;

    // not working todo fix
    //echo "destination = " . $destination;
    $form_state['values']['imgurl'] = $destination;

    if (file_copy($file, $destination, FILE_EXISTS_REPLACE)) {
      $form_state['values']['picture'] = $file->filepath;
    }
    else {
      form_set_error('picture_upload', t("Failed to upload the banner image; the %directory directory doesn't exist or is not writable.", array('%directory' => "/sites/all/theme/banners")));
    }
  }
}

/**
 * Save a new Banner to the database
 */
function dynamic_banner_default_admin_form_submit($form, &$form_state) {
  // remember text is optional. save the default banner
  dynamic_banner_set_banner($form_state['values']['path'], $form_state['values']['imgurl'], $form_state['values']['text'], $form_state['values']['link'], 999);

  drupal_set_message(t('The default banner has been saved.'));
  $form_state['redirect'] = 'admin/structure/banners';
  return;
}

/**
 * Return a form for editing or creating an individual Banner
 *
 * ingroup forms
 * see path_admin_form_validate()
 * see path_admin_form_submit()
 */
function dynamic_banner_admin_form($form, &$form_state, $edit = array('path' => '', 'imgurl' => '', 'text' => '', 'link' => '', 'dbid' => NULL)) {
  $form['#banner'] = $edit;

  $form['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Existing system path'),
    '#default_value' => $edit['path'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the existing path you wish to put a banner on. For example: home, user* (wild card), content! (random).'),
    '#field_prefix' => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q='),
    '#required' => TRUE,
  );

  /********* TODO MAKE LOGIC FOR IMAGE UPLOADER AND CHOOSER!!! *************/
  $form['imgurl'] = array(
    '#type' => 'textfield',
    '#title' => t('Image Url'),
    '#default_value' => $edit['imgurl'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the path to the image. Starting from the root site directory(comma seperated for randoms).'),
    '#field_prefix' => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q='),
    //'#required' => TRUE,
  );

  // banner picture chooser
  $form['picture'] = array(
    '#type' => 'fieldset',
    '#title' => t('Banner'),
  );

  $picture = $edit['imgurl'];
  //if we are editing an existing pictue place that code in here
  if ($picture) {
    $form['picture']['current_picture'] = array('#value' => "<img src='/" . $picture . "'/>");
    $form['picture']['picture_delete'] = array(
      '#type' => 'checkbox',
      '#title' => t('Delete Banner'),
      '#description' => t('Check this box to delete your current picture.'),
    );
  }
  else {
    $form['picture']['picture_delete'] = array('#type' => 'hidden');
  }
  $form['picture']['picture_upload'] = array(
    '#type' => 'file',
    '#title' => t('Upload Banner'),
    '#size' => 64,
    //'#description' => t('Maximum dimensions are %dimensions and the maximum size is %size kB.', array('%dimensions' => '800x800', '%size' => '1024')),
  );
  $form['#validate'][] = 'dynamic_banner_validate_picture';

  $form['text'] = array(
    '#type' => 'textfield',
    '#title' => t('Text'),
    '#default_value' => $edit['text'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the text to associate with this banner (comma seperated for randoms) (also must match amount of elements from images) -optional.'),
    '#required' => FALSE,
  );
  $form['link'] = array(
    '#type' => 'textfield',
    '#title' => t('Link'),
    '#default_value' => $edit['link'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the link you want your banner to point to (optional).'),
    '#field_prefix' => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q='),
    '#required' => FALSE,
  );
  if ($edit['dbid']) {
    $form['dbid'] = array(
      '#type' => 'hidden',
      '#value' => $edit['dbid'],
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Update Banner'),
    );
  }
  else {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Create a new Banner'),
    );
  }

  return $form;
}

/**
 * Verify that the Banner is valid
 */
function dynamic_banner_admin_form_validate($form, &$form_state) {
  $path = $form_state['values']['path'];
  //$imgurl = $form_state['values']['imgurl'];
  //$picture = $form_state['values']['picture']['picture_upload'];
  //$link = $form_state['values']['link'];
  //$dbid = isset($form_state['values']['dbid']) ? $form_state['values']['dbid'] : 0;

  // we are updating so let it make the changes
  if ( $dbid != 0) {
    return;
  }

  // check for more than one of the same path banners
  if (db_query("SELECT COUNT(path) FROM {dynamic_banner} WHERE path = :path", array(':path' => $path))->fetchField()) {
    form_set_error('path', t('The path %path is already in use.', array('%path' => $path)));
  }

  /* TODO FIX VALIDATION ERRORS*/
  /*
   // check for path existance and path access
   if ( !drupal_valid_path($path)) {
   form_set_error('path', t("The path '@link_path' is either invalid or you do not have access to it.", array('@link_path' => $path)));
   }*/
}

/**
 * Set a banner for a given path, preventing duplicates.
 * Note if dbid comes in null then we are creating a banner
 */
function dynamic_banner_set_banner($path = NULL, $imgurl = NULL, $text = NULL, $link = NULL, $dbid = NULL) {
  $path = urldecode($path);
  $imgurl = urldecode($imgurl);
  // First we check if we deal with an existing alias and delete or modify it based on dbid.
  if ($dbid) {
    /* TODO FIX LOGIC */
    // An existing alias.
    /* if (!$path || !$alias) {
     // Delete the alias based on pid.
     db_query('DELETE FROM {dynamic_banner} WHERE dbid = %d', $dbid);
     } */
    // the default
    if ( $dbid == 999 ) {
      /* db_query("INSERT INTO {dynamic_banner} (path, imgurl, text, link, dbid) VALUES ('%s', '%s', '%s', '%s, '%d')", $path, $imgurl, $text, $link, $dbid) */
      $id = db_insert('dynamic_banner')->fields(array(
        'path' => $path,
        'imgurl' => $imgurl,
        'text' => $text,
        'link' => $link,
        'dbid' => $dbid,
      ))->execute();
    }
    else {
      // Update the existing banner.
      /* db_query("UPDATE {dynamic_banner} SET path = '%s', imgurl = '%s', text = '%s', link = '%s' WHERE dbid = %d", $path, $imgurl, $text, $link, $dbid) */
      db_update('dynamic_banner')->fields(array(
        'path' => $path,
        'imgurl' => $imgurl,
        'text' => $text,
        'link' => $link,
      ))->condition('dbid', $dbid)->execute();
    }
  }
  else {
    /* TODO FIX LOGIC */
    // Check for existing banner path.
    /* there is currently no such method for banners such as path alias
     if ($banner == drupal_get_path_alias($path, $language)) {
     // There is already such an alias, neutral or in this language.
     // Update the alias based on alias; setting the language if not yet done.
     db_query("UPDATE {dynamic_banner} SET path = '%s', imgurl = '%s', text = '%s' WHERE path = '%s'", $path, $imgurl, $text, $path);
     }
     else {*/
    // A new alias. Add it to the database.
    /* db_query("INSERT INTO {dynamic_banner} (path, imgurl, text, link) VALUES ('%s', '%s', '%s', '%s')", $path, $imgurl, $text, $link) */
    $id = db_insert('dynamic_banner')->fields(array(
      'path' => $path,
      'imgurl' => $imgurl,
      'text' => $text,
      'link' => $link,
    ))->execute();
    //}
  }
}

/**
 * Save a new Banner to the database.
 */
function dynamic_banner_admin_form_submit($form, &$form_state) {
  // remember text is optional.
  dynamic_banner_set_banner($form_state['values']['path'], $form_state['values']['imgurl'], $form_state['values']['text'], $form_state['values']['link'], isset($form_state['values']['dbid']) ? $form_state['values']['dbid'] : 0);

  drupal_set_message(t('The banner has been saved.'));
  $form_state['redirect'] = 'admin/structure/banners';
  return;
}

/**
 * Menu callback; confirms deleting a Banner
 */
function dynamic_banner_admin_delete_confirm($form, $form_state, $dbid) {
  $path = dynamic_banner_load($dbid);
  if (user_access('administer dynamic_banner')) {
    $form['dbid'] = array(
      '#type' => 'value',
      '#value' => $dbid,
    );
    $output = confirm_form($form, 
      t('Are you sure you want to delete banner %title?', array('%title' => $path['path'])), 
      isset($_GET['destination']) ? $_GET['destination'] : 'admin/structure/banners');
  }
  return $output;
}

/**
 * Execute banners deletion
 */
function dynamic_banner_admin_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    dynamic_banner_admin_delete($form_state['values']['dbid']);
    $form_state['redirect'] = 'admin/structure/banners';
    return;
  }
}

/**
 * Post-confirmation; delete a Banner
 */
function dynamic_banner_admin_delete($dbid = 0) {
  db_delete('dynamic_banner')->condition('dbid', $dbid)->execute();
  drupal_set_message(t('The banner has been deleted, the image still exists though'));
}

/**
 * Return a form to filter Banners.
 *
 * ingroup forms
 * see dynamic_banner_admin_filter_form_submit()
 */
function dynamic_banner_admin_filter_form($form) {
  $filters = dynamic_banner_filters();
  
  $form['filters'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Filter dynamic banner'), 
    '#collapsible' => TRUE, 
    '#collapsed' => empty($_SESSION['dynamic_banner_filter']),
  );
  
  foreach ($filters as $key => $filter) {
    $form['filters']['status'][$key] = array(
      '#title' => $filter['title'], 
      '#type' => 'select', 
      '#multiple' => TRUE, 
      '#size' => 8, 
      '#options' => $filter['options'],
    );
    if (!empty($_SESSION['dynamic_banner_filter'][$key])) {
      $form['filters']['status'][$key]['#default_value'] = $_SESSION['dynamic_banner_filter'][$key];
    }
  }
  
  $form['filters']['actions'] = array(
    '#type' => 'actions', 
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['filters']['actions']['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Filter'),
  );
  if (!empty($_SESSION['dynamic_banner_filter'])) {
    $form['filters']['actions']['reset'] = array(
      '#type' => 'submit', 
      '#value' => t('Reset'),
    );
  }

  return $form;
}

/**
 * Validate result from dynamic banner administrative filter form.
 */
function dynamic_banner_admin_filter_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Filter') && empty($form_state['values']['type'])) {
    form_set_error('type', t('You must select something to filter by.'));
  }
}

/**
 * Process result from dynamic banner administrative filter form.
 */
function dynamic_banner_admin_filter_form_submit($form, &$form_state) {
  $op = $form_state['values']['op'];
  $filters = dblog_filters();
  switch ($op) {
    case t('Filter'):
      foreach ($filters as $name => $filter) {
        if (isset($form_state['values'][$name])) {
          $_SESSION['dynamic_banner_filter'][$name] = $form_state['values'][$name];
        }
      }
      break;
    case t('Reset'):
      $_SESSION['dynamic_banner_filter'] = array();
      break;
  }
  return 'admin/structure/banners/list/';
}

/**
 * Process filter form submission when the Reset button is pressed.
 */
function dynamic_banner_admin_filter_form_submit_reset($form, &$form_state) {
  $form_state['redirect'] = 'admin/structure/banners/list';
}

/**
 * Fetch a specific banner from the database.
 */
function dynamic_banner_load($dbids) {
  foreach ($dbids as $nid => &$dbid) {
    $node_additions = db_fetch_array(db_query("SELECT * FROM {dynamic_banner} WHERE dbid = :dbid", array(':dbid' => $dbid)));
    foreach ($node_additions as $property => &$value) {
      $dbid->$property = $value;
    }
  }
}

/**
 * A page that will display a form for changing how dynamic banner will function
 */
function dynamic_banner_settings($form, $form_state) {
  // todo need to find a better way of doing this
  $display_setting = variable_get( 'dynamic_banner_display_setting', DEFAULT_OUTPUT );
  if ( $display_setting == 'url' ) {
    $display_current_setting = 0;
  }
  elseif ( $display_setting == 'text' ) {
    $display_current_setting = 1;
  }
  elseif ( $display_setting == 'urltext' ) {
    $display_current_setting = 2;
  }
  elseif ( $display_setting == 'urllink' ) {
    $display_current_setting = 3;
  }

  // todo need to find a better way of doing this
  $errors_current_setting = 1;
  if ( variable_get('dynamic_banner_display_errors', DEFAULT_ERROR) ) {
    $errors_current_setting = 0;
  }

  $form['display_setting'] = array(
    '#type' => 'radios',
    '#title' => t('Display Setting'),
    '#options' => array(t('url'), t('text'), t('urltext'), t('urllink')),
    '#default_value' => $display_current_setting,
    '#description' => t('What display pattern do you want the module to follow in the template file'),
  );

  $form['display_errors'] = array(
    '#type' => 'radios',
    '#title' => t('Display Errors?'),
    '#options' => array(t('yes'), t('no')),
    '#default_value' => $errors_current_setting,
    '#description' => t('If dynamic banner can not find a banner for the current page do you want it to display an error?'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * when the settings form submits this function will save the settings for use
 */
function dynamic_banner_settings_submit($form, &$form_state) {
  $display = $form_state['values']['display_setting'];

  // todo fix this
  if ($display == 0) {
    $display = 'url';
  }
  elseif ($display == 1) {
    $display = 'text';
  }
  elseif ($display == 2) {
    $display = 'urltext';
  }
  elseif ($display == 3) {
    $display = 'urllink';
  }
  variable_set('dynamic_banner_display_setting', $display);

  $errors = $form_state['values']['display_errors'];

  // todo fix this
  if ($errors == 0) {
    variable_set('dynamic_banner_display_errors', TRUE);
  }
  else {
    variable_set('dynamic_banner_display_errors', FALSE);
  }

  $form_state['redirect'] = '/admin/build/banners/';
}
