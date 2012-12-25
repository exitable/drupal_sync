<?php

/**
 * @file
 * drupal_sync.entity.inc
 */

/**
 * Implements hook_entity_insert().
 */
function drupal_sync_entity_insert($entity, $type) {
  drupal_sync_entity_prepare_to_queue($entity, $type, 'insert');
}

/**
 * Implements hook_entity_update().
 */
function drupal_sync_entity_update($entity, $type) {
  drupal_sync_entity_prepare_to_queue($entity, $type, 'update');
}


/**
 * Implements hook_entity_delete().
 */
function drupal_sync_entity_delete($entity, $type) {
  drupal_sync_entity_prepare_to_queue($entity, $type, 'delete');
}


/**
 * Prepare entity for adding to queue
 * @param stdClass $entity
 * @param string $type
 * @param string $op
 */
function drupal_sync_entity_prepare_to_queue($entity, $type, $op = 'insert') {
  // Insert the new entity into a fictional table of all entities.
  $drupal_sync_settings = variable_get('drupal_sync_settings', array());

  $entities = (isset($drupal_sync_settings['drupal_sync_entities']) && !empty($drupal_sync_settings['drupal_sync_entities'])) ? $drupal_sync_settings['drupal_sync_entities'] : array();
  $remotes = (isset($drupal_sync_settings['drupal_sync_remote']) && !empty($drupal_sync_settings['drupal_sync_remote'])) ? $drupal_sync_settings['drupal_sync_remote'] : array();


  if (!empty($remotes) && array_key_exists($type, $entities)) {
    foreach ($remotes as $remote) {
      $entity->remote_url = $remote['url'];
      // send to all remote sites
      switch ($type) {
        case 'node':
          // check if $entity->type in settings
          if (isset($entities[$type][$entity->type])) {
            // send node to remote sites
            drupal_sync_add_to_queue($entity->nid, $type, (array) $entity, $op);
          }
          break;
        case 'taxonomy':
          // check if $entity->type in settings
          if (isset($entities[$type][$entity->vid])) {
            // send term to remote sites
            drupal_sync_add_to_queue($entity->tid, $type, (array) $entity, $op);
          }
          break;
      }
    }
  }
}