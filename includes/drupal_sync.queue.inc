<?php

/**
 * @file
 * drupal_sync.queue.inc
 */

/**
 * Implements hook_drupal_sync_entity_field_types_info().
 */
function drupal_sync_drupal_sync_entity_field_types_info($types = array()) {
  $types['text_formatted'] = array('value');
  $types['date'] = array('value', 'value2', 'offset', 'offset2', 'timezone', 'show_todate', 'date_type', 'timezone_db');
  $types['field_item_link'] = array('url', 'title');
  $types['field_item_image'] = array('fid', 'alt', 'title', 'description');
  $types['list<text>'] = array('value');
  $types['list<decimal>'] = array('value');
  $types['integer'] = array('value');
  $types['boolean'] = array('value');
  $types['decimal'] = array('value');
  $types['text'] = array('value');
  $types['field_item_file'] = array('fid', 'description');
  $types['struct'] = array('value', 'value2', 'offset', 'offset2', 'timezone', 'show_todate', 'date_type', 'timezone_db', 'rrule');

  return $types;
}

/**
 * Add sync task to queue
 *
 * @param int $local_eid
 * @param string $type
 * @param array $entity
 * @param string $op
 *
 * @return int
 */
function drupal_sync_add_to_queue($local_eid, $type, $entity, $op) {
  $drupal_sync_relation_settings = variable_get('drupal_sync_remote_relation_settings', array());
  $remote_site_info = drupal_sync_get_remote_site_access($entity['remote_url']);
  $entity_property_info = entity_get_property_info($type);

  $ref = $drupal_sync_relation_settings[$type][$entity['type']][$remote_site_info['key']]['ref'];
  $properties = (isset($entity_property_info['bundles'][$entity['type']]['properties'])) ? $entity_property_info['bundles'][$entity['type']]['properties'] : array();

  $types = array();
  $types = module_invoke_all('drupal_sync_entity_field_types_info', $types);

  $opt = array();
  foreach ($ref as $local_field => $remote_field) {
    $opt[$remote_field] = array();
    $local_field_type = (isset($properties[$local_field]['type'])) ? $properties[$local_field]['type'] : NULL;

    if (!empty($local_field_type)) {
      $sync_field_fields = (isset($types[$local_field_type])) ? $types[$local_field_type] : array('value');
      $local_field_values = (isset($entity[$local_field][LANGUAGE_NONE])) ? $entity[$local_field][LANGUAGE_NONE] : array();
      foreach ($local_field_values as $local_field_key => $local_field_value) {
        foreach ($sync_field_fields as $sync_field_field) {
          if (isset($local_field_value[$sync_field_field])) {
            $opt[$remote_field][$local_field_key][$sync_field_field] = $local_field_value[$sync_field_field];
            // add file url
            if ($sync_field_field == 'fid') {
              $fid = $local_field_value[$sync_field_field];
              $file = file_load($fid);
              $opt[$remote_field][$local_field_key]['file_url'] = file_create_url($file->uri);
            }
          }
        }
      }
    }
    else {
      $opt[$remote_field] = (isset($entity[$local_field])) ? $entity[$local_field] : ''; // for titles
    }
  }

  $entity_data = array(
    'entity_id' => $local_eid,
    'entity_content_type' => $entity['type'],
    'entity_fields' => $opt,
  );

  return db_insert('drupal_sync_queue')
          ->fields(array(
            'remote_site_id' => $entity['remote_url'],
            'entity_id' => $local_eid,
            'entity_type' => $type,
            'entity' => serialize($entity_data),
            'operation' => $op,
            'created' => REQUEST_TIME
          ))
          ->execute();
}

/**
 * Get sync task from queue
 *
 * @return array
 */
function drupal_sync_get_from_queue() {
  $drupal_sync_queue_update_count = variable_get('drupal_sync_queue_update_county', 10); // in seconds
  return db_select('drupal_sync_queue', 'dsq')
          ->fields('dsq', array())
          ->range(0, $drupal_sync_queue_update_count)
          ->orderBy('id', 'ASC')
          ->execute()->fetchAll(PDO::FETCH_ASSOC);
}

/**
 * Delete sync task from queue
 *
 * @param int $id
 *
 * @return int
 */
function drupal_sync_delete_from_queue($id) {
  return db_delete('drupal_sync_queue')
          ->condition('id', $id)
          ->execute();
}

/**
 * Run sync operation
 */
function drupal_sync_run_sync() {
  $queue_entities = drupal_sync_get_from_queue();
  foreach ($queue_entities as $queue_entity) {
    $entity = unserialize($queue_entity['entity']);

    // send entity
    $remote_eid = drupal_sync_xmlrpc_sync_entity($queue_entity['remote_site_id'], $queue_entity['entity_type'], $entity, $queue_entity['operation']);
    if ($remote_eid) {
      if($queue_entity['operation'] == 'insert') {
        switch($queue_entity['operation']) {
          case 'insert':
            drupal_sync_local_store_insert($queue_entity['entity_id'], $remote_eid, $queue_entity['entity_type'], $queue_entity['remote_site_id']);
            break;
          case 'delete':
            drupal_sync_local_store_delete($queue_entity['entity_id'], $remote_eid, $queue_entity['entity_type'], $queue_entity['remote_site_id']);
            break;
        }

      }
      drupal_sync_delete_from_queue($queue_entity['id']);
    }
  }
}

function drupal_sync_test() {
  drupal_sync_run_sync();
}

