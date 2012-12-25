<?php

/**
 * @file
 * Settings forms
 */


/**
 * Implements hook_drupal_sync_prepare_types().
 */
function drupal_sync_drupal_sync_prepare_types($types) {
  $node_types = node_type_get_types();
  foreach ($node_types as $key => $value) {
    $types['node'][$key] = array(
      'type' => $value->type,
      'name' => $value->name
    );
  }

  $taxonomy_voc = taxonomy_get_vocabularies();
  foreach ($taxonomy_voc as $key => $value) {
    $types['taxonomy'][$value->machine_name] = array(
      'vid' => $value->vid,
      'name' => $value->name
    );
  }
  return $types;
}


/**
 * Implements hook_drupal_sync_default_fields_info().
 */
function drupal_sync_drupal_sync_default_fields_info($fields) {
  $fields['node'] = array(
    'title' => array(
      'label' => t('Title')
     ),
  );

  $fields['taxonomy'] = array(
    'name' => array(
      'label' => t('Name'),
     ),
    'description' => array(
      'label' => t('Description'),
     ),
  );

  return $fields;
}



/**
 * Implements hook_drupal_sync_types_build_form().
 */
function drupal_sync_drupal_sync_types_build_form($form, $types, $defaults = array()) {
    foreach ($types as $type => $value) {
    switch ($type) {
      case 'node':
        $options = array();
        foreach ($value as $option) {
          $options[$option['type']] = $option['name'];
        }

        $form['drupal_sync_settings']['drupal_sync_entities']['drupal_sync_node_settings'] = array(
          '#type' => 'fieldset',
          '#title' => t('Available node types '),
          '#collapsible' => TRUE,
        );

        $form['drupal_sync_settings']['drupal_sync_entities']['drupal_sync_node_settings']['drupal_sync_node_types'] = array(
          '#type' => 'checkboxes',
          '#title' => t('Choose node types for sync'),
          '#default_value' => (isset($defaults['node'])) ? array_keys($defaults['node']) : array(),
          '#options' => $options,
        );

        break;
      case 'taxonomy':
        $options = array();
        foreach ($value as $option) {
          $options[$option['vid']] = $option['name'];
        }
        $form['drupal_sync_settings']['drupal_sync_entities']['drupal_sync_taxonomy_settings'] = array(
          '#type' => 'fieldset',
          '#title' => t('Available taxonomy vocabularies '),
          '#collapsible' => TRUE,
        );

        $form['drupal_sync_settings']['drupal_sync_entities']['drupal_sync_taxonomy_settings']['drupal_sync_taxonomy_vocabularies'] = array(
          '#type' => 'checkboxes',
          '#title' => t('Choose vocabularies for sync'),
          '#default_value' => (isset($defaults['taxonomy'])) ? array_keys($defaults['taxonomy']) : array(),
          '#options' => $options,
        );
        break;
    }
  }
  return $form;
}

/**
 * Implements hook_drupal_sync_settings_submit_form().
 */
function drupal_sync_drupal_sync_settings_submit_form($data, $form_state) {
  $settings = (isset($form_state['values']['drupal_sync_settings'])) ? $form_state['values']['drupal_sync_settings'] : array();

  $node_settings = isset($settings['drupal_sync_entities']['drupal_sync_node_settings']['drupal_sync_node_types']) ? $settings['drupal_sync_entities']['drupal_sync_node_settings']['drupal_sync_node_types'] : array();
  $node_settings = array_filter($node_settings);

  foreach ($node_settings as $key => $value) {
    $node_settings[$key] = (isset($form_state['original_entity_types']['node'][$key]['name'])) ? $form_state['original_entity_types']['node'][$key]['name'] : $value;
  }

  $taxonomy_settings = isset($settings['drupal_sync_entities']['drupal_sync_taxonomy_settings']['drupal_sync_taxonomy_vocabularies']) ? $settings['drupal_sync_entities']['drupal_sync_taxonomy_settings']['drupal_sync_taxonomy_vocabularies'] : array();
  $taxonomy_settings = array_filter($taxonomy_settings);

  $original_taxonomy = (isset($form_state['original_entity_types']['taxonomy'])) ? $form_state['original_entity_types']['taxonomy'] : array();
  $tmp_taxonomy = array();
  foreach ($original_taxonomy as $machine_name => $value) {
    $tmp_taxonomy[$value['vid']] = array(
      'name' => $value['name'],
      'machine_name' => $machine_name
    );
  }
  $original_taxonomy = $tmp_taxonomy;

  foreach ($taxonomy_settings as $key => $value) {
    $taxonomy_settings[$key] = (isset($original_taxonomy[$key]['name'])) ? $original_taxonomy[$key]['name'] : $value;
  }

  $data['drupal_sync_entities'] = array(
    'node' => $node_settings,
    'taxonomy' => $taxonomy_settings
  );
  return $data;
}

/**
 * Create settings form in drupal configuration setings section for module
 */
function drupal_sync_admin_settings_form($form, &$form_state) {
  $request_limit = drupal_map_assoc(array(1, 3, 10, 20, 30, 50, 100));
  $data = variable_get('drupal_sync_settings', array());
  $types = array();
  $types = module_invoke_all('drupal_sync_prepare_types', $types);
  $form_state['original_entity_types'] = $types;

  $queue_update_frequency = drupal_map_assoc(array(60, 300, 600, 1800, 3600), 'format_interval');
  $queue_update_count = drupal_map_assoc(array(1, 3, 5, 10, 20, 50, 100, 200, 300, 500, 1000));


  /*   * ***************************************************************
   * Base section
   * *************************************************************** */
  $form['#tree'] = TRUE;
  $form['#attached']['css'][] = drupal_get_path('module', 'drupal_sync') . '/css/drupal_sync_admin.css';

  $form['drupal_sync_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sync settings'),
  );

  $form['drupal_sync_settings']['drupal_sync_request_limit'] = array(
    '#type' => 'select',
    '#title' => t('Count http requests (if receive fail)'),
    '#default_value' => variable_get('drupal_sync_request_limit', 3),
    '#options' => $request_limit,
    '#required' => TRUE
  );

  $form['drupal_sync_settings']['drupal_sync_queue_update_frequency'] = array(
    '#type' => 'select',
    '#title' => t('Frequency of updates'),
    '#default_value' => variable_get('drupal_sync_queue_update_frequency', 60),
    '#options' => $queue_update_frequency,
    '#required' => TRUE
  );

  $form['drupal_sync_settings']['drupal_sync_queue_update_count'] = array(
    '#type' => 'select',
    '#title' => t('Could of updates per request'),
    '#default_value' => variable_get('drupal_sync_queue_update_count', 10),
    '#options' => $queue_update_count,
    '#required' => TRUE
  );

  $form['drupal_sync_settings']['drupal_sync_entities'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sync entities'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );


  /*   * ***************************************************************
   * Entities section
   * *************************************************************** */
  $defaults = isset($data['drupal_sync_entities']) ? $data['drupal_sync_entities'] : array();
  $form = module_invoke_all('drupal_sync_types_build_form', $form, $types, $defaults);

  /*   * ***************************************************************
   * Sync auth section
   * *************************************************************** */
  $form['drupal_sync_settings']['drupal_sync_auth_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sync auth'),
    '#description' => t('Access for sync content from other sites to current site.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['drupal_sync_settings']['drupal_sync_auth_settings']['drupal_sync_login'] = array(
    '#type' => 'textfield',
    '#title' => t('Curent site login'),
    '#default_value' => isset($data['drupal_sync_auth_settings']['drupal_sync_login']) ? $data['drupal_sync_auth_settings']['drupal_sync_login'] : '',
  );

  $form['drupal_sync_settings']['drupal_sync_auth_settings']['drupal_sync_pass'] = array(
    '#type' => 'textfield',
    '#title' => t('Curent site password'),
    '#default_value' => isset($data['drupal_sync_auth_settings']['drupal_sync_pass']) ? $data['drupal_sync_auth_settings']['drupal_sync_pass'] : '',
  );


  /*   * ***************************************************************
   * Sync remote section
   * *************************************************************** */

  $form['drupal_sync_settings']['drupal_sync_remote'] = array(
    '#type' => 'fieldset',
    '#title' => t('Remote sites'),
    '#description' => t('Send updates from current site to remote sites.'),
    '#prefix' => '<div id="drupal-sync-remote-wrapper">',
    '#suffix' => '</div>',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  if (empty($form_state['drupal_sync_remote_num'])) {
    $form_state['drupal_sync_remote_num'] = (isset($data['drupal_sync_remote_count'])) ? $data['drupal_sync_remote_count'] : 1;
  }
  for ($i = 0; $i < $form_state['drupal_sync_remote_num']; $i++) {
    $form['drupal_sync_settings']['drupal_sync_remote'][$i] = array(
      '#type' => 'fieldset',
      '#title' => t('Site ' . ($i + 1)),
      '#collapsible' => TRUE,
    );

    $form['drupal_sync_settings']['drupal_sync_remote'][$i]['drupal_sync_remote_url'] = array(
      '#type' => 'textfield',
      '#title' => t('Url'),
      '#default_value' => (isset($data['drupal_sync_remote'][$i]['url'])) ? $data['drupal_sync_remote'][$i]['url'] : '',
    );

    $form['drupal_sync_settings']['drupal_sync_remote'][$i]['drupal_sync_remote_login'] = array(
      '#type' => 'textfield',
      '#title' => t('Login'),
      '#default_value' => (isset($data['drupal_sync_remote'][$i]['login'])) ? $data['drupal_sync_remote'][$i]['login'] : '',
    );

    $form['drupal_sync_settings']['drupal_sync_remote'][$i]['drupal_sync_remote_pass'] = array(
      '#type' => 'textfield',
      '#title' => t('Password'),
      '#default_value' => (isset($data['drupal_sync_remote'][$i]['pass'])) ? $data['drupal_sync_remote'][$i]['pass'] : '',
    );
  }

  $form['drupal_sync_settings']['drupal_sync_remote']['drupal_sync_add_name'] = array(
    '#type' => 'submit',
    '#value' => t('Add one more'),
    '#submit' => array('drupal_sync_remote_add_one'),
    '#ajax' => array(
      'callback' => 'drupal_sync_remote_add_more_callback',
      'wrapper' => 'drupal-sync-remote-wrapper',
    ),
  );
  if ($form_state['drupal_sync_remote_num'] > 1) {
    $form['drupal_sync_settings']['drupal_sync_remote']['drupal_sync_remove_name'] = array(
      '#type' => 'submit',
      '#value' => t('Remove one'),
      '#submit' => array('drupal_sync_remote_remove_one'),
      '#ajax' => array(
        'callback' => 'drupal_sync_remote_add_more_callback',
        'wrapper' => 'drupal-sync-remote-wrapper',
      ),
    );
  }

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save'));
  $form['#submit'][] = 'drupal_sync_admin_settings_form_submit';
  return $form;
}

/**
 * Submit handler for the "add-one-more" button.
 *
 * Increments the max counter and causes a rebuild.
 */
function drupal_sync_remote_add_one($form, &$form_state) {
  $form_state['drupal_sync_remote_num']++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Callback for both ajax-enabled buttons.
 *
 * Selects and returns the fieldset with the names in it.
 */
function drupal_sync_remote_add_more_callback($form, $form_state) {
  return $form['drupal_sync_settings']['drupal_sync_remote'];
}

/**
 * Submit handler for the "remove one" button.
 *
 * Decrements the max counter and causes a form rebuild.
 */
function drupal_sync_remote_remove_one($form, &$form_state) {
  if ($form_state['drupal_sync_remote_num'] > 1) {
    $form_state['drupal_sync_remote_num']--;
  }
  $form_state['rebuild'] = TRUE;
}

/**
 * Form drupal_sync_admin_settings_form Validation
 */
function drupal_sync_admin_settings_form_validate($form, &$form_state) {
  $url_values = array();
  for ($i = 0; isset($form_state['values']['drupal_sync_settings']['drupal_sync_remote'][$i]); $i++) {
    $url_values[$i] = $form_state['values']['drupal_sync_settings']['drupal_sync_remote'][$i]['drupal_sync_remote_url'];
  }
  $url_duplicates = array();
  $duplicate_values = _drupal_sync_get_duplicates_values($url_values);
  if (!empty($duplicate_values)) {
    //search url duplicates
    foreach ($url_values as $i => $value) {
      if (in_array($value, $duplicate_values)) {
        form_set_error('drupal_sync_settings][drupal_sync_remote][' . $i . '][drupal_sync_remote_url', 'Error, site URLs should be unique');
      }
    }
  }
}

/**
* Takes an array and returns an array of duplicate items
*/
function _drupal_sync_get_duplicates_values( $array ) {
	return array_diff_assoc( $array, array_unique( $array ) );
}

/**
 * Form drupal_sync_admin_settings_form submit function
 * @param type $form
 * @param type $form_state
 */
function drupal_sync_admin_settings_form_submit($form, &$form_state) {
  $data = array();
  $settings = (isset($form_state['values']['drupal_sync_settings'])) ? $form_state['values']['drupal_sync_settings'] : array();

  $data['drupal_sync_request_limit'] = $settings['drupal_sync_request_limit'];
  $data['drupal_sync_auth_settings'] = $settings['drupal_sync_auth_settings'];
  $data['drupal_sync_remote_count'] = (isset($form_state['drupal_sync_remote_num'])) ? $form_state['drupal_sync_remote_num'] : 1;

  $data = module_invoke_all('drupal_sync_settings_submit_form', $data, $form_state);

  $remote = (isset($settings['drupal_sync_remote'])) ? $settings['drupal_sync_remote'] : array();
  for ($i = 0; $i < $data['drupal_sync_remote_count']; $i++) {
    if (isset($remote[$i])) {
      $data['drupal_sync_remote'][$i] = array(
        'url' => (isset($remote[$i]['drupal_sync_remote_url'])) ? $remote[$i]['drupal_sync_remote_url'] : '',
        'login' => (isset($remote[$i]['drupal_sync_remote_login'])) ? $remote[$i]['drupal_sync_remote_login'] : '',
        'pass' => (isset($remote[$i]['drupal_sync_remote_pass'])) ? $remote[$i]['drupal_sync_remote_pass'] : '',
      );
    }
  }

  variable_set('drupal_sync_settings', $data);

  // clear auth cache
  variable_del('drupal_sync_xmlrpc_auth_options');
}