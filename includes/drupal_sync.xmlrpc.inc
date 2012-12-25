<?php

/**
 * Implements hook_xmlrpc().
 *
 * @return array
 */
function drupal_sync_xmlrpc() {
  $methods[] = array(
    'drupal_sync_xmlrpc.auth',
    'drupal_sync_xmlrpc_auth',
    array('array', 'string', 'string'),
    t('User auth')
  );

  $methods[] = array(
    'drupal_sync_xmlrpc.get_sync_types',
    'drupal_sync_xmlrpc_get_sync_types',
    array('array'),
    t('Get types for sync')
  );

  $methods[] = array(
    'drupal_sync_xmlrpc.sync',
    'drupal_sync_xmlrpc_sync',
    array('array', 'string', 'array', 'string', 'string'),
    t('Sync entity from remote site')
  );

  $methods[] = array(
    'drupal_sync_xmlrpc.set_entity_sync_settings',
    'drupal_sync_xmlrpc_set_entity_sync_settings',
    array('array', 'array'),
    t('Set entity syncronization settings')
  );

  return $methods;
}

/**
 * Set entity syncronization settings
 *
 * @param array $settings
 */
function drupal_sync_xmlrpc_set_entity_sync_settings($settings) {

  $drupal_sync_settings = variable_get('drupal_sync_settings', array());
  $drupal_sync_relation_settings = variable_get('drupal_sync_remote_relation_settings', array());

  $remote_site_info = drupal_sync_get_remote_site_access($settings['site_url']);
  if (!empty($remote_site_info)) {
    $enity_type = $settings['entity_type'];
    $enity_type_id = $settings['reciever_entity_id'];
    $site_key = $remote_site_info['key'];

    if (isset($drupal_sync_settings['drupal_sync_entities'][$enity_type][$enity_type_id])) {
      if ($settings['settings']['drupal_sync_both'] == 0) {
        //change only drupal_sync_both flag to 0
        $drupal_sync_relation_settings[$enity_type][$enity_type_id][$site_key]['drupal_sync_both'] = 0;
      }
      else {
        $drupal_sync_relation_settings[$enity_type][$enity_type_id][$site_key] = $settings['settings'];
      }
      //save settings
      variable_set('drupal_sync_remote_relation_settings', $drupal_sync_relation_settings);
    }
    else {
      return array(
        'status' => 'error',
        'message' => t('Check remote site sync settings, there is no settings for given entity-type: @enity_type entity-id: @enity_type_id', array('@enity_type' => $enity_type, '@enity_type_id' => $enity_type_id)),
      );
    }
  }
  else {
    return array(
      'status' => 'error',
      'message' => t('Check remote site sync settings, there is no settings for given url: @url', array('@url' => $settings['site_url']))
    );
  }

  return array(
    'status' => 'ok',
  );
}

/**
 * Xmlrpc authorization
 *
 * @global stdClass $user
 * @param string $login
 * @param string $pass
 *
 * @return stdClass
 */
function drupal_sync_xmlrpc_auth($login, $pass) {
  global $user;
  $uid = user_authenticate($login, $pass);
  if ($uid) {
    $user = user_load($uid);
    user_login_finalize();
    return array('session_name' => session_name(), 'sessid' => session_id());
  }
  return $user;
}

/**
 * Sync entities
 * @param array $options
 * @param stdClass $entity
 * @return array
 */
function drupal_sync_xmlrpc_sync($type = 'node', $entity = array(), $op = 'insert', $remote_url) {
  $result = array();
  if (user_access('access drupal sync')) {
    $result = module_invoke_all('drupal_sync_xmlrpc_sync_alert', $entity, $type, $op, $remote_url, $result);
  }
  return $result;
}

function drupal_sync_xmlrpc_get_sync_types() {
  $types = array();
  if (user_access('access drupal sync')) {
    $types = module_invoke_all('drupal_sync_get_sync_types', $types);
  }
  return $types;
}

/* * ********************************************************************
 * xmlrpc api
 * ******************************************************************** */

/**
 * Custom http request
 *
 * @staticvar int $c
 * @param String $url
 *
 * @return String
 */
function drupal_sync_xmlrpc_send($url = '', $args = array(), $auth_data = array()) {
  static $c;
  $result = NULL;
  $limit = variable_get('drupal_sync_request_limit', 10);
  while ((!isset($result) || empty($result)) && $c <= $limit) {
    $c++;
    $options = variable_get('drupal_sync_xmlrpc_auth_options', array());
    if (empty($options)) {
      $auth = drupal_sync_auth($url, $auth_data['login'], $auth_data['pass']);
      if ($auth) {
        $options = array();
        if (!empty($auth['sessid'])) {
          $options = array('headers' => array('Cookie' => $auth['session_name'] . '=' . $auth['sessid']));
        }
        variable_set('drupal_sync_xmlrpc_auth_options', $options);
      }
    }

    $result = drupal_sync_xmlrpc_request($url, $args, $options);

    // do re-auth
    if (empty($result)) {
      variable_del('drupal_sync_xmlrpc_auth_options');
    }
  }
  return $result;
}

/**
 * Custom http request
 *
 * @staticvar int $c
 * @param String $url
 *
 * @return String
 */
function drupal_sync_xmlrpc_request($url, $args = array(), $options = array()) {
  $url .= '/xmlrpc.php';
  $result = xmlrpc($url, $args, $options);
  return $result;
}

/**
 * xmlrpc prepare request: Auth via drupal xmlrpc
 * @param string $url
 * @param string $login
 * @param string $pass
 * @return array
 */
function drupal_sync_auth($url = '', $login = '', $pass = '') {
  // auth
  $args = array(
    'drupal_sync_xmlrpc.auth' => array(
      $login,
      $pass,
    ),
  );
  return drupal_sync_xmlrpc_request($url, $args, array());
}


/**
 * xmlrpc prepare request: Send entity to remote sites.
 * @param array $remote
 * @param string $type
 * @param array $entity
 * @param string $op
 * @return type
 */
function drupal_sync_xmlrpc_sync_entity($remote_site_id, $type, $entity, $op = 'insert') {
  global $base_url;
  $remote = drupal_sync_get_remote_site_access($remote_site_id);

  $url = (isset($remote['url']) && !empty($remote['url'])) ? $remote['url'] : FALSE;
  $login = (isset($remote['login']) && !empty($remote['login'])) ? $remote['login'] : FALSE;
  $pass = (isset($remote['pass']) && !empty($remote['pass'])) ? $remote['pass'] : FALSE;

  $args = array(
    'drupal_sync_xmlrpc.sync' => array(
      $type, $entity, $op, $base_url
    ),
  );
  return drupal_sync_xmlrpc_send($url, $args, array('login' => $login, 'pass' => $pass));
}

/**
 * xmlrpc prepare request: Send entity syncronizatio settings to the remote site.
 *
 * @param string $remote_site_id
 * @param array $settings
 * @return type
 */
function drupal_sync_xmlrpc_send_entity_sync_settings($remote_site_id, $settings) {

  $remote = drupal_sync_get_remote_site_access($remote_site_id);
  $url = (isset($remote['url']) && !empty($remote['url'])) ? $remote['url'] : FALSE;
  $login = (isset($remote['login']) && !empty($remote['login'])) ? $remote['login'] : FALSE;
  $pass = (isset($remote['pass']) && !empty($remote['pass'])) ? $remote['pass'] : FALSE;

  $args = array(
    'drupal_sync_xmlrpc.set_entity_sync_settings' => array(
      $settings,
    ),
  );
  return drupal_sync_xmlrpc_send($url, $args, array('login' => $login, 'pass' => $pass));
}

/**
 * xmlrpc prepare request: Get remote config
 * @param string $url
 * @param string $login
 * @param string $pass
 * @return array()
 */
function drupal_sync_get_remote_config($url = '', $login = '', $pass = '') {
  $args = array(
    'drupal_sync_xmlrpc.get_sync_types' => array(),
  );
  return drupal_sync_xmlrpc_send($url, $args, array('login' => $login, 'pass' => $pass));
}

/**
 * Get remote site access by site URL
 * @param string $remote_site_id
 * @return array
 */
function drupal_sync_get_remote_site_access($remote_site_id) {
  $drupal_sync_settings = variable_get('drupal_sync_settings', array());
  if (isset($drupal_sync_settings['drupal_sync_remote']) && !empty($drupal_sync_settings['drupal_sync_remote'])) {
    foreach ($drupal_sync_settings['drupal_sync_remote'] as $site_key => $remote_site) {
      if ($remote_site['url'] == $remote_site_id) {
        $remote_site['key'] = $site_key;
        return $remote_site;
      }
    }
  }
  return FALSE;
}

/* * ********************************************************************
 * HOOKS
 * ******************************************************************** */

/**
 * Implements of hook_drupal_sync_get_sync_types().
 */
function drupal_sync_drupal_sync_get_sync_types($types) {
  $drupal_sync_settings = variable_get('drupal_sync_settings', array());
  $types = (isset($drupal_sync_settings['drupal_sync_entities'])) ? $drupal_sync_settings['drupal_sync_entities'] : array();

  $fields = array();
  $fields = module_invoke_all('drupal_sync_default_fields_info', $fields);

  foreach ($types as $type => $entity) {
    switch ($type) {
      case 'node':
        foreach ($entity as $key => $value) {
          $default_fields = (isset($fields['node'])) ? $fields['node'] : array();
          $current_fields = $default_fields + field_info_instances('node', $key);
          $types[$type][$key] = array(
            'name' => $value,
            'fields' => $current_fields
          );
        }
        break;
      case 'taxonomy':
        foreach ($entity as $key => $value) {
          $default_fields = (isset($fields['taxonomy'])) ? $fields['taxonomy'] : array();
          $voc = taxonomy_vocabulary_load($key);
          $current_fields = $default_fields + field_info_instances('taxonomy_term', $voc->machine_name);
          $types[$type][$key] = array(
            'name' => $value,
            'fields' => $current_fields
          );
        }
        break;
    }
  }
  return $types;
}