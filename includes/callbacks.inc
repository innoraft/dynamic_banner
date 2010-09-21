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
function dynamic_banner_admin_page($keys = NULL) {  
  // Add the filter form above the overview table.
  $output = drupal_get_form('dynamic_banner_admin_filter_form', $keys);
  $count = db_result(db_query("SELECT COUNT(*) FROM {dynamic_banner}"));

  if ($keys) {
    // Replace wildcards with MySQL/PostgreSQL wildcards.
    $keys = preg_replace('!\*+!', '%', $keys);
    $sql = "SELECT * FROM {dynamic_banner} WHERE path LIKE '%%%s%%'";
  }
  else {
    $sql = 'SELECT * FROM {dynamic_banner}';
  }
  
  // construct the headers of the table
  $header = array(
    array('data' => t('Url'), 'field' => 'path', 'sort' => 'asc'),
    array('data' => t('ImgUrl'), 'field' => 'imgurl'),
    array('data' => t('Text'), 'field' => 'text'),
    array('data' => t('Operations'), 'colspan' => '2')
  );
  
  // allows us to sort the table
  $sql .= tablesort_sql($header);
  // get the current page of results
  $result = pager_query($sql, 50, 0 , NULL, $keys);

  // start constructing the individual rows
  $rows = array();
  while ($data = db_fetch_object($result)) {
    if ( $data->dbid == 999) {
      $row = array(check_plain('DEFAULT'), check_plain($data->imgurl), check_plain($data->text), l(t('edit'), "admin/build/banners/default"), l(t('delete'), "admin/build/banners/delete/$data->dbid"));
    }
    else {
      $row = array(check_plain($data->path), check_plain($data->imgurl), check_plain($data->text), l(t('edit'), "admin/build/banners/edit/$data->dbid"), l(t('delete'), "admin/build/banners/delete/$data->dbid"));
    }
    $rows[] = $row;
  }

  // deal with an empty set
  if (empty($rows)) {
    $empty_message = $keys ? t('No Banners Found.') : t('No Banner Urls found.') ;
    $rows[] = array(array('data' => $empty_message));
  }

  // display the information
  $output .= theme('table', $header, $rows);
  $output .= theme('pager', NULL, 50, 0);

  // let drupal handle print and echo
  return $output;
}


/**
 * Menu callback; handles pages for creating and editing banners
 */
function dynamic_banner_admin_edit($dbid = 0) {
  if ($dbid) {
    $banner = dynamic_banner_load($dbid);
    drupal_set_title(check_plain($banner['path']));
    $output = drupal_get_form('dynamic_banner_admin_form', $banner);
  }
  else {
    $output = drupal_get_form('dynamic_banner_admin_form');
  }

  return $output;
}

/**
 * Menu callback; handles pages for creating and editing the default banner (999(
 */
function dynamic_banner_default_admin_edit($dbid = 999) {
  if ($dbid) {
    $banner = dynamic_banner_load($dbid);
    drupal_set_title(check_plain($banner['path']));
    $output = drupal_get_form('dynamic_banner_default_admin_form', $banner);
  }
  else {
    $output = drupal_get_form('dynamic_banner_default_admin_form');
  }

  return $output;
}

function dynamic_banner_default_admin_form(&$form_state, $edit = array('path' => '', 'imgurl' => '', 'text' => '')) {
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
    '#required' => TRUE,
  );
  $form['text'] = array(
    '#type' => 'textfield',
    '#title' => t('Text'),
    '#default_value' => $edit['text'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the text to associate with this banner (comma seperated for randoms) (also must match amount of elements from images) -optional.'),
    '#required' => FALSE,
  );
  $form['submit'] = array('#type' => 'submit', '#value' => t('Update Banner'));

  return $form;
}

/**
 * Save a new Banner to the database.
 */
function dynamic_banner_default_admin_form_submit($form, &$form_state) {
  // remember text is optional. save the default banner
  dynamic_banner_set_banner($form_state['values']['path'], $form_state['values']['imgurl'], $form_state['values']['text'], 999);

  drupal_set_message(t('The default banner has been saved.'));
  $form_state['redirect'] = 'admin/build/banners';
  return;
}

/**
 * Return a form for editing or creating an individual Banner
 *
 * ingroup forms
 * see path_admin_form_validate()
 * see path_admin_form_submit()
 */
function dynamic_banner_admin_form(&$form_state, $edit = array('path' => '', 'imgurl' => '', 'text' => '', 'dbid' => NULL)) {
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
    '#required' => TRUE,
  );
  $form['text'] = array(
    '#type' => 'textfield',
    '#title' => t('Text'),
    '#default_value' => $edit['text'],
    '#maxlength' => 128,
    '#size' => 45,
    '#description' => t('Specify the text to associate with this banner (comma seperated for randoms) (also must match amount of elements from images) -optional.'),
    '#required' => FALSE,
  );
  if ($edit['dbid']) {
    $form['dbid'] = array('#type' => 'hidden', '#value' => $edit['dbid']);
    $form['submit'] = array('#type' => 'submit', '#value' => t('Update Banner'));
  }
  else {
    $form['submit'] = array('#type' => 'submit', '#value' => t('Create a new Banner'));
  }

  return $form;
}

/**
 * Verify that the Banner is valid
 */
function dynamic_banner_admin_form_validate($form, &$form_state) {
  $path = $form_state['values']['path'];
  $imgurl = $form_state['values']['imgurl'];
  $dbid = isset($form_state['values']['dbid']) ? $form_state['values']['dbid'] : 0;
  
  // we are updating so let it make the changes
  if ( $dbid != 0) {
      return;
  }

  // check for more than one of the same path banners
  if (db_result(db_query("SELECT COUNT(path) FROM {dynamic_banner} WHERE path = '%s'", $path))) {
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
function dynamic_banner_set_banner($path = NULL, $imgurl = NULL, $text = NULL, $dbid = NULL) {
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
      db_query("INSERT INTO {dynamic_banner} (path, imgurl, text, dbid) VALUES ('%s', '%s', '%s', '%d')", $path, $imgurl, $text, $dbid);
    }
    else {
      // Update the existing banner.
      db_query("UPDATE {dynamic_banner} SET path = '%s', imgurl = '%s', text = '%s' WHERE dbid = %d", $path, $imgurl, $text, $dbid);
    }
  }
  elseif ($path && $imgurl) {
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
    db_query("INSERT INTO {dynamic_banner} (path, imgurl, text) VALUES ('%s', '%s', '%s')", $path, $imgurl, $text);
    //}
  }
  else {
    // Delete the alias.
    if ($imgurl) {
      db_query("DELETE FROM {dynamic_banner} WHERE imgurl = '%s'", $imgurl);
    }
    else {
      db_query("DELETE FROM {dynamic_banner} WHERE path = '%s'", $path);
    }
  }
}

/**
 * Save a new Banner to the database.
 */
function dynamic_banner_admin_form_submit($form, &$form_state) {
  // remember text is optional.
  dynamic_banner_set_banner($form_state['values']['path'], $form_state['values']['imgurl'], $form_state['values']['text'], isset($form_state['values']['dbid']) ? $form_state['values']['dbid'] : 0);

  drupal_set_message(t('The banner has been saved.'));
  $form_state['redirect'] = 'admin/build/banners';
  return;
}

/**
 * Menu callback; confirms deleting a Banner
 */
function dynamic_banner_admin_delete_confirm($form_state, $dbid) {
  $path = dynamic_banner_load($dbid);
  if (user_access('administer dynamic_banner')) {
    $form['dbid'] = array('#type' => 'value', '#value' => $dbid);
    $output = confirm_form($form,
      t('Are you sure you want to delete banner %title?', array('%title' => $path['path'])),
      isset($_GET['destination']) ? $_GET['destination'] : 'admin/build/banners');
    }
  return $output;
}

/**
 * Execute banners deletion
 */
function dynamic_banner_admin_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    dynamic_banner_admin_delete($form_state['values']['dbid']);
    $form_state['redirect'] = 'admin/build/banners';
    return;
  }
}


/**
 * Post-confirmation; delete a Banner
 */
function dynamic_banner_admin_delete($dbid = 0) {
  db_query('DELETE FROM {dynamic_banner} WHERE dbid = %d', $dbid);
  drupal_set_message(t('The banner has been deleted, the image still exists though'));
}


/**
 * Return a form to filter Banners.
 *
 * ingroup forms
 * see dynamic_banner_admin_filter_form_submit()
 */
function dynamic_banner_admin_filter_form(&$form_state, $keys = '') {
  $form['#attributes'] = array('class' => 'search-form');
  
  $form['basic'] = array('#type' => 'fieldset',
    '#title' => t('Filter Banners')
  );
  
  $form['basic']['inline'] = array('#prefix' => '<div class="container-inline">', '#suffix' => '</div>');
  
  $form['basic']['inline']['filter'] = array(
    '#type' => 'textfield',
    '#title' => '',
    '#default_value' => $keys,
    '#maxlength' => 128,
    '#size' => 25,
  );
  
  $form['basic']['inline']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#submit' => array('dynamic_banner_admin_filter_form_submit_filter'),
  );
  
  if ($keys) {
    $form['basic']['inline']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
    '#submit' => array('dynamic_banner_admin_filter_form_submit_reset'),
    );
  }
  return $form;
}

/**
 * Process filter form submission when the Filter button is pressed.
 */
function dynamic_banner_admin_filter_form_submit_filter($form, &$form_state) {
  $form_state['redirect'] = 'admin/build/banners/list/'. trim($form_state['values']['filter']);
}

/**
 * Process filter form submission when the Reset button is pressed.
 */
function dynamic_banner_admin_filter_form_submit_reset($form, &$form_state) {
  $form_state['redirect'] = 'admin/build/banners/list';
}


/**
 * Helper function for grabbing filter keys.
 */
function dynamic_banner_admin_filter_get_keys() {
  // Extract keys as remainder of path
  $path = explode('/', $_GET['q'], 5);
  return count($path) == 5 ? $path[4] : '';
}

/**
 * Fetch a specific URL alias from the database.
 */
function dynamic_banner_load($dbid) {
  return db_fetch_array(db_query("SELECT * FROM {dynamic_banner} WHERE dbid = '%d'", $dbid));
}

function dynamic_banner_settings($form_state) {
  // todo need to find a better way of doing this
  $display_setting = variable_get( 'dynamic_banner_display_setting', '' );
  if ( $display_setting == 'url' ) {
	$display_current_setting = 0;
  } elseif ( $display_setting == 'text' ) {
    $display_current_setting = 1;
  } elseif ( $display_setting == 'urltext' ) {
    $display_current_setting = 2;
  }
  
  // todo need to find a better way of doing this
  $errors_current_setting = 1;
  if ( variable_get('dynamic_banner_display_errors', '') ) {
     $errors_current_setting = 0;
  }
  
  // grab the current view from the user
  $mode = arg(3);

  $form['display_setting'] = array(
    '#type'          => 'radios',
    '#title'         => t('Display Setting'),
    '#options'       => array(t('url'), t('text'), t('urltext')),
    '#default_value' => $display_current_setting,
    '#description'   => t('What display pattern do you want the module to follow in the template file'),
  );

  $form['display_errors'] = array(
    '#type'          => 'radios',
    '#title'         => t('Display Errors?'),
    '#options'       => array(t('yes'), t('no')),
    '#default_value' => $errors_current_setting,
    '#description'   => t('If dynamic banner can not find a banner for the current page do you want it to display an error?'),
  );
  
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Save'),
  );

  
  return $form;
}

function dynamic_banner_settings_submit($form, &$form_state) {
  $display = $form_state['values']['display_setting'];
  
  // todo fix this
  if ($display == 0) {
    $display = 'url';
  } elseif ($display == 1) {
	$display = 'text';
  } elseif ($display == 2) {
    $display = 'urltext';
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