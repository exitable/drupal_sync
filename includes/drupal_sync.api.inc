<?php

/**
 * @file
 * drupal_sync.api.inc
 */

/* * *************************************************************************
 * CLIENT side
 * ************************************************************************* */

/**
 * Implements of  drupal_sync_xmlrpc_sync_alert().
 */
function drupal_sync_drupal_sync_xmlrpc_sync_alert($entity, $type, $op, $remote_url, $result) {
  $ref_fields = array(); //@TODO: fix for choose fields
  switch ($type) {
    case 'node':
      $ref_fields = _drupal_sync_get_fields_refs($type, $entity['entity_content_type'], $remote_url);
      switch ($op) {
        case 'insert':
          $nid = drupal_sync_entity_node_insert($entity, $ref_fields);
          drupal_sync_local_store_insert($nid, $entity['entity_id'], $type, $remote_url);
          return $nid;
          break;
        case 'update':
          $nid = drupal_sync_entity_node_update($entity, $ref_fields, $remote_url);
          return $nid;
          break;
        case 'delete':
          $nid = drupal_sync_entity_node_delete($entity, $ref_fields, $remote_url);
          drupal_sync_local_store_delete($nid, $entity['entity_id'], $type, $remote_url);
          return $nid;
          break;
      }
      break;
    case 'taxonomy':
      $ref_fields = _drupal_sync_get_fields_refs($type, $entity['vid'], $remote_url);
      switch ($op) {
        case 'insert':
          $tid = drupal_sync_entity_term_insert($entity, $ref_fields);
          drupal_sync_local_store_insert($tid, $entity['tid'], $type, $entity['remote_url']);
          return $tid;
          break;
        case 'update':
          $tid = drupal_sync_entity_term_update($entity, $ref_fields);
          return $tid;
          break;
        case 'delete':
          $tid = drupal_sync_entity_term_delete($entity, $ref_fields);
          drupal_sync_local_store_delete($tid, $entity['tid'], $type, $entity['remote_url']);
          return $tid;
          break;
      }
      break;
  }
  return FALSE;
}

/* * *****************************************************************
 * Node operations
 * ***************************************************************** */

/**
 * Insert local node.
 *
 * @param array $entity
 * @param array $ref_fields
 *
 * @return int
 */
function drupal_sync_entity_node_insert($entity, $ref_fields) {
  // insert node.
//  watchdog('sync_insert_ent', '<pre>' . print_r($entity, TRUE) . '</pre>');
//  watchdog('sync_insert_fld', '<pre>' . print_r($ref_fields, TRUE) . '</pre>');

 // $drupal_sync_relation_settings = variable_get('drupal_sync_remote_relation_settings', array());
  $entity_property_info = entity_get_property_info('node');

  $properties = (isset($entity_property_info['bundles'][$entity['entity_content_type']]['properties'])) ? $entity_property_info['bundles'][$entity['entity_content_type']]['properties'] : array();
  $types = array();
  $types = module_invoke_all('drupal_sync_entity_field_types_info', $types);

  $node = new stdClass();
  $node->type = $entity['entity_content_type'];
  node_object_prepare($node); // Sets some defaults. Invokes hook_prepare() and hook_node_prepare().
  $node->language = LANGUAGE_NONE;

  foreach ($entity['entity_fields'] as $field_key => $values) {
    if ($field_key == 'title') {
      $node->$field_key = $values;
    }
    else {
      $local_field_type = (isset($properties[$field_key]['type'])) ? $properties[$field_key]['type'] : NULL;

      if (!empty($local_field_type)) {
        $sync_field_fields = (isset($types[$local_field_type])) ? $types[$local_field_type] : array('value');
        $local_field_values = (isset($values) && is_array($values)) ? $values : array();
        foreach ($local_field_values as $local_field_key => $local_field_value) {
          $field_value = array();
          foreach ($sync_field_fields as $sync_field_field) {
            if (isset($local_field_value[$sync_field_field])) {
              $field_value[LANGUAGE_NONE][$local_field_key][$sync_field_field] = array_shift($local_field_value);
            }
          }
          $node->$field_key = $field_value;
        }
      }
    }
  }

  $node->status = 1; //(1 or 0): published or not
  $node->promote = 0; //(1 or 0): promoted to front page
  $node->comment = 1; //2 = comments on, 1 = comments off

  node_save($node);
  //watchdog('sync_inserted', '<pre>' . print_r($node, TRUE) . '</pre>');
  return $node->nid;
}


/**
 * Update local node.
 *
 * @param array $entity
 * @param array $ref_fields
 *
 * @return int
 */
function drupal_sync_entity_node_update($entity, $ref_fields, $remote_url) {
  // update node.
  $nid = drupal_sync_local_store_get_id($entity['entity_id'], $remote_url);

  $node = node_load($nid);

  $entity_property_info = entity_get_property_info('node');

  $properties = (isset($entity_property_info['bundles'][$entity['entity_content_type']]['properties'])) ? $entity_property_info['bundles'][$entity['entity_content_type']]['properties'] : array();
  $types = array();
  $types = module_invoke_all('drupal_sync_entity_field_types_info', $types);

  foreach ($entity['entity_fields'] as $field_key => $values) {
    if ($field_key == 'title') {
      $node->$field_key = $values;
    } else {
      $local_field_type = (isset($properties[$field_key]['type'])) ? $properties[$field_key]['type'] : NULL;

      if (!empty($local_field_type)) {
        $sync_field_fields = (isset($types[$local_field_type])) ? $types[$local_field_type] : array('value');
        $local_field_values = (isset($values) && is_array($values)) ? $values : array();
        foreach ($local_field_values as $local_field_key => $local_field_value) {
          $field_value = array();
          foreach ($sync_field_fields as $sync_field_field) {
            if (isset($local_field_value[$sync_field_field])) {
              $field_value[LANGUAGE_NONE][$local_field_key][$sync_field_field] = array_shift($local_field_value);
            }
          }
          $node->$field_key = $field_value;
        }
      }
    }
  }

  node_save($node);

  return $nid;
}

/**
 * Delete local node.
 *
 * @param array $entity
 * @param array $ref_fields
 *
 * @return int
 */
function drupal_sync_entity_node_delete($entity, $ref_fields, $remote_url) {
  // delete node.
  $nid = drupal_sync_local_store_get_id($entity['entity_id'], $remote_url);
  node_delete($nid);
  return $nid;
}

/* * *****************************************************************
 * Taxonomy operations
 * ***************************************************************** */

/**
 * Insert local node.
 *
 * @param array $entity
 * @param array $ref_fields
 *
 * @return int
 */
function drupal_sync_entity_term_insert($entity, $ref_fields) {
  // insert term.
  $tid = 1;
  return $tid;
}

/**
 * Update local node.
 *
 * @param array $entity
 * @param array $ref_fields
 *
 * @return int
 */
function drupal_sync_entity_term_update($entity, $ref_fields) {
  // update term.
  $tid = 1;
  return $tid;
}

/**
 * Delete local node.
 *
 * @param array $entity
 * @param array $ref_fields
 *
 * @return int
 */
function drupal_sync_entity_term_delete($entity, $ref_fields) {
  // delete term.
  $tid = 1;
  return $tid;
}

/* * ********************************************************************
 * Local sync store
 * ******************************************************************** */

/**
 * Insert row to local sync store
 *
 * @param int $nid
 * @param int $rnid
 *
 * @param string $type
 */
function drupal_sync_local_store_insert($eid, $reid, $type, $remote_url) {
  // instert to local store
  return db_insert('drupal_sync')
          ->fields(array(
            'eid' => $eid,
            'reid' => $reid,
            'entity_type' => $type,
            'site_id' => $remote_url,
          ))
          ->execute();
}

/**
 * Delete row from local sync store
 *
 * @param int $nid
 * @param int $rnid
 * @param string $type
 * @param string $remote_url
 *
 * @param string $type
 */
function drupal_sync_local_store_delete($eid, $reid, $type, $remote_url) {
  // delete from local store
  return db_delete('drupal_sync')
          ->condition('eid', $eid)
          ->condition('reid', $reid)
          ->condition('entity_type', $type)
          ->condition('site_id', $remote_url)
          ->execute();
}

/**
 * Get local entity id by remote id
 *
 * @param int $nid
 * @param int $rnid
 *
 * @param string $type
 */
function drupal_sync_local_store_get_id($reid, $remote_url) {
   return db_select('drupal_sync', 's')
          ->fields('s', array('eid'))
          ->condition('reid', $reid)
          ->condition('site_id', $remote_url)
          ->execute()
          ->fetchField();
}

/**
 * Get field mapping from settings
 *
 * @param string $entity_type
 * @param string $entity_type_id
 * @param string $site_url
 *
 * @return array
 */
function _drupal_sync_get_fields_refs($entity_type, $entity_type_id, $site_url) {
  $fields_refs = array();

  $remote_site_info = drupal_sync_get_remote_site_access($site_url);
  if(isset($remote_site_info['key'])) {
    $drupal_sync_relation_settings = variable_get('drupal_sync_remote_relation_settings', array());
    $site_key = $remote_site_info['key'];
    foreach ($drupal_sync_relation_settings[$entity_type] as $enity_type_id => $remote_settings) {
      if(!empty($remote_settings[$site_key]['type']) && $remote_settings[$site_key]['type'] == $entity_type_id) {
        $remote_settings[$site_key]['local_entity_type'] = $enity_type_id;
        $fields_refs[$enity_type_id] = $remote_settings[$site_key];
      }
    }
  }

  return $fields_refs;
}