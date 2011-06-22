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
  
  // Add the filter form above the overview table. // fix bug first //
  //$output['dynamic_banner_admin_filter_form'] = drupal_get_form('dynamic_banner_admin_filter_form');
  
  // construct the headers of the table
  $header = array(
    array(
      'data'  => t('Url'),
      'field' => 'd.path',
      'sort'  => 'asc',
    ),
    array(
      'data'  => t('ImgUrl'),
      'field' => 'd.imgurl',
    ),
    array(
      'data'  => t('Text'),
      'field' => 'd.text',
    ),
    array(
      'data'  => t('Link'),
      'field' => 'd.link',
    ),
    array(
      'data'    => t('Operations'),
      'colspan' => '2',
    ),
  );

  // contruct the db call for the list
  // load all data fields and attach pager and sorter function in
  $query = db_select('dynamic_banner', 'd')->extend('PagerDefault')->extend('TableSort'); // alias the table name to d
  $query->fields('d') // Select all fields from the node table. 
    ->limit(20) // Load the first 20 rows.
    ->orderByHeader($header);
  
  // find if the filter has returned a where clause and add it in before executing
  if (!empty($filter['where'])) {
    $query->where($filter['where'], $filter['args']);// not working yet
  }
  $result = $query->execute();

  // start constructing the individual rows
  $rows = array();
  foreach ($result as $data) {
    $rows[] = array('data' => 
      array(
        $data->path,
        $data->imgurl,
        $data->text,
        $data->link,
        l(t('edit'), "admin/structure/banners/edit/". $data->dbid),
        l(t('delete'), "admin/structure/banners/delete/" . $data->dbid)
      )
    );
  }
  
  // construct the call for the theme function to run on this
  $output['dynamic_banner_table'] = array(
    '#theme'  => 'table', 
    '#header' => $header, 
    '#rows'   => $rows, 
    '#empty'  => t('No Banners Found.'),
  );
  
  // adds the pager buttons to the bottom of the table
  $output['dynamic_banner_pager'] = array('#theme' => 'pager');

  // let drupal handle print and echo
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
 * The specific filters that can be used for banners
 */
function dynamic_banner_filters() {
  $filters = array();
  $filters['type'] = array(
    'title'   => t('Type'),
    'where'   => 'd.url ?',
    'options' => array('NOT LIKE %* AND NOT LIKE %!','LIKE %*','LIKE %!'));
  return $filters;
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
    '#type'        => 'fieldset', 
    '#title'       => t('Filter dynamic banner'), 
    '#collapsible' => TRUE, 
    '#collapsed'   => empty($_SESSION['dynamic_banner_filter']),
  );
  
  foreach ($filters as $key => $filter) {
    $form['filters']['status'][$key] = array(
      '#title'    => $filter['title'], 
      '#type'     => 'select', 
      '#multiple' => TRUE, 
      '#size'     => 8, 
      '#options'  => $filter['options'],
    );
    if (!empty($_SESSION['dynamic_banner_filter'][$key])) {
      $form['filters']['status'][$key]['#default_value'] = $_SESSION['dynamic_banner_filter'][$key];
    }
  }
  
  $form['filters']['actions'] = array(
    '#type'       => 'actions', 
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['filters']['actions']['submit'] = array(
    '#type'  => 'submit', 
    '#value' => t('Filter'),
  );
  if (!empty($_SESSION['dynamic_banner_filter'])) {
    $form['filters']['actions']['reset'] = array(
      '#type'  => 'submit', 
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
 * The main form dealing with dynamic banner 
 * There is now only one form for dynamic banner to deal with unlink in the d6 version
 *
 * INPUT: arg(4) is from the url path of this form
 */
function dynamic_banner_admin_form($form, &$form_state) {
  $dbid = arg(4); // the last portion of the url there must be a better way of doing this
  
  $file_path = drupal_get_path('module', 'file');
  
  // default the variable here
  $banner = NULL; // if i dont do this bugs happen
  
  if ($dbid != NULL) {
    if ($dbid == 0) {
      drupal_set_title(t('New Banner'));
    }
    else {
      $banner = dynamic_banner_load_banner($dbid);
      drupal_set_title(t("Edit Banner") . " '" . $banner->path . "'");
    }
  }
  //$form['banner'] = $edit;
  //$form['banner']['#access'] = FALSE;

  $form['#attributes'] = array('enctype' => "multipart/form-data");
  
  $form['path'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Banner Path'),
    '#default_value' => $banner ? $banner->path : "",
    '#size'          => 45,
    '#description'   => t('Specify an existing url path you wish to put a banner on. For example: home, user* (wild card), content! (random).'),
    '#field_prefix'  => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q='),
    '#required'      => TRUE,
  );
  
  /**
   * Note: There are two form elements for the same thing 
   * They are both not required but only one is needed for proper handling
   * When we are loading an old banner load the url into imgurl
   * When we are uploading a new image the validator will upload the image store it and fill in imgurl for you
   * Only use one method no mix and matching
   * When reading the data use checks to see which method was used
   */
  $form['imgurl'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Typeout the url of the image'),
    '#default_value' => $banner ? $banner->imgurl : '',
    '#description'   => t('Specify an image(s) for the banner to display.'),
    '#field_prefix'  => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q='),
    //'#required'    => TRUE,
  );
  
  $form['image'] = array(
    '#title'              => t('Choose Image File'),
    '#type'               => 'managed_file',
    '#default_value'      => $banner ? $banner->imgfid : '',
    '#attached'           => array(
      'js' => array($file_path . '/file.js'),
     ),
    '#progress_indicator' => 'throbber',
    '#progress_message'   => NULL,
    '#upload_location'    => variable_get('dynamic_banner_file_save_path', DEFAULT_SAVE_LOCATION),
    '#description'        => t('Specify an image(s) for the banner to display.'),
    //'#required'         => TRUE,
  );
  $form['#validate'][] = 'dynamic_banner_upload_image_validate';

  $form['text'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Text'),
    '#default_value' => $banner ? $banner->text : '',
    '#maxlength'     => 128,
    '#size'          => 45,
    '#description'   => t('Specify the text to associate with this banner (comma seperated for randoms) (also must match amount of elements from images) -optional.'),
    '#required'      => FALSE,
  );

  $form['link'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Link'),
    '#default_value' => $banner ? $banner->link : '',
    '#maxlength'     => 128,
    '#size'          => 45,
    '#description'   => t('Specify the link you want your banner to point to (optional).'),
    '#required'      => FALSE,
  );
  
  $form['time_on'] = array(
    '#type'          => 'date',
    '#title'         => t('Start Time'),
    '#description'   => t('Specify the time you want your banner to start displaying (optional).'),
    '#required'      => FALSE,
  );
  
  $form['time_off'] = array(
    '#type'          => 'date',
    '#date_format'   => 'h:i A',
    '#title'         => t('End Time'),
    '#description'   => t('Specify the time you want your banner to stop displaying (optional).'),
    '#required'      => FALSE,
  );
  
  $form['mode'] = array(
    '#type'          => 'radios',
    '#title'         => t('Mode'),
    '#options'       => array(t('normal'), t('time_based'), t('rotating'), t('fade')),
    '#default_value' => DEFAULT_BANNER_MODE,
    '#description'   => t('What mode do you want this banner to display under (this is different than display setting)'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Banner'),
  );

  return $form;
}

/**
 * Validate/submit handler used for handling image uploads
 */
function dynamic_banner_upload_image_validate($form, &$form_state) {
  // This is not needed, I use this to use the same validate function
  // for several fields.
  /*$file = file_save_upload($form['values']['image'], array(
    'file_validate_is_image'   => array(),
    'file_validate_extensions' => array('png gif jpg jpeg'),
  ));*/
  $file = file_load($form_state['values']['image']);
  
  if ($file) {
    // Get the image info to get the correct extension for the uploaded file.
    /*$info = image_get_info($file->filepath);
    if (file_move($file, 'destination/filename'. $info['extension'], FILE_EXISTS_REPLACE)) {
      // Mark the file for permanent storage.
      file_set_status($file, FILE_STATUS_PERMANENT);
      // Update the files table.
      drupal_write_record('files', $file, 'fid');*/
    // Change status to permanent.
    $file->status = FILE_STATUS_PERMANENT;
    
    // When a module is managing a file, it must manage the usage count.
    // Here we increment the usage count with file_usage_add().
    file_usage_add($file, 'dynamic_banner', 'banner', 1);
    $form_state['values']['image'] = $file->filepath;
    
    // Save.
    file_save($file);
  }
  else {
    form_set_error($form_state['values']['image'], t('Failed to write the uploaded file to the folder.'));
  }
}


/**
 * Save a new Banner to the database
 */
function dynamic_banner_admin_form_submit($form, &$form_state) {
  // Add Validation Checks Here
  $path     = $form_state['values']['path'];
  $imgurl   = $form_state['values']['imgurl'];
  $text     = $form_state['values']['text'];
  $link     = $form_state['values']['link'];
  $mode     = $from_state['values']['mode'];
  $time_on  = $from_state['values']['mode'];
  $time_off = $from_state['values']['mode'];
  ////////////////TODO ////////////////
  
  // Save the banner
  dynamic_banner_set_banner($path, $imgurl, $text, $link, 999);

  drupal_set_message(t('The banner has been saved.'));
  $form_state['redirect'] = 'admin/structure/banners';
  return;
}

/**
 * Verify that the Banner is valid
 */
function dynamic_banner_admin_form_validate($form, &$form_state) {
  $path = $form_state['values']['path'];
  $imgurl = $form_state['values']['imgurl'];
  $picture = $form_state['values']['picture']['picture_upload'];
  $link = $form_state['values']['link'];
  $dbid = isset($form_state['values']['dbid']) ? $form_state['values']['dbid'] : 0;

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
function dynamic_banner_set_banner($path, $imgurl, $text, $link, $dbid, $mode, $time_on, $time_off) {
  dpm($path);
  dpm($imgurl);
  $path = urldecode($path);
  $imgurl = urldecode($imgurl);
  if (!$mode) {
    $mode = DEFAULT_BANNER_MODE;
  }
  dpm("after url decode");
  dpm($path);
  dpm($imgurl);
  // First we check if we are dealing with an existing alias and delete or modify it based on dbid.
  // we dont need to do a complicated check here because the code already made it for us
  if ($dbid) {
    // Update the existing banner.
    db_update('dynamic_banner')->fields(array(
      'path'       => $path,
      'imgurl'     => $imgurl,
      'text'       => $text,
      'link'       => $link,
      'mode'       => $mode,
      'start_time' => $time_on,
      'end_time'   => $time_off,
    ))->condition('dbid', $dbid)->execute();
  }
  else {
    db_insert('dynamic_banner')->fields(array(
      'path'       => $path,
      'imgurl'     => $imgurl,
      'text'       => $text,
      'link'       => $link,
      'mode'       => $mode,
      'start_time' => $time_on,
      'end_time'   => $time_off,
    ))->execute();
  }
}

/**
 * Menu callback; confirms deleting a Banner
 */
function dynamic_banner_admin_delete_confirm($form, $form_state, $dbid) {
  $banner = dynamic_banner_load_banner($dbid);
  if (user_access('administer dynamic_banner')) {
    $form['dbid'] = array(
      '#type' => 'value',
      '#value' => $dbid,
    );
    $output = confirm_form($form, 
      t('Are you sure you want to delete banner %title?', array('%title' => $banner->path)), 
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
 * Fetch a specific banner from the database.
 */
function dynamic_banner_load_banner($dbid) {
  $query = db_select('dynamic_banner', 'd');
  $query->condition('d.dbid', $dbid, '=')
    ->fields('d');
  $result = $query->execute()->fetchObject();
  
  if ($result) {
    return $result;
  }
  return NULL;
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
    '#type'          => 'radios',
    '#title'         => t('Display Setting'),
    '#options'       => array(t('url'), t('text'), t('urltext'), t('urllink')),
    '#default_value' => $display_current_setting,
    '#description'   => t('What display pattern do you want the module to follow in the template file'),
  );

  $form['display_errors'] = array(
    '#type'          => 'radios',
    '#title'         => t('Display Errors?'),
    '#options'       => array(t('yes'), t('no')),
    '#default_value' => $errors_current_setting,
    '#description'   => t('If dynamic banner can not find a banner for the current page do you want it to display an error?'),
  );// does this still work when there is a default banner
  
  $form['image_save_path'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Image save path'),
    '#default_value' => variable_get('dynamic_banner_file_save_path', DEFAULT_SAVE_LOCATION),
    '#description'   => t('This will be the path all banners get saved to when using the upload utility.'),
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
  
  variable_set('dynamic_banner_file_save_path', $form_state['values']['image_save_path']);

  $form_state['redirect'] = '/admin/build/banners/';
}
