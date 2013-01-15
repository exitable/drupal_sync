<?php

/**
 * @file
 * Settings forms
 */

/**
 * Create settings form in drupal configuration setings section for module
 */
function drupal_sync_initial_setup_admin_settings_form($form, &$form_state) {
  $node_types = node_type_get_types();
  $vocs = taxonomy_vocabulary_get_names();

  $voc_options = array();
  foreach ($vocs as $voc) {
    $voc_options[$voc->vid] = $voc->name;
  }

  $node_types = node_type_get_types();
  $node_options = array();
  foreach ($node_types as $node_type) {
    $node_options[$node_type->type] = $node_type->name;
  }

  $data = variable_get('drupal_sync_initial_setup_settings', array('node_types' => array(), 'vocabularies' => array()));

  $form['node_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Choose content types for setup relations.'),
    '#options' => $node_options,
    '#default_value' => isset($data['node_types']) ? $data['node_types'] : array(),
  );

  $form['vocabularies'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Choose vocabularies for setup relations.'),
    '#options' => $voc_options,
    '#default_value' => isset($data['vocabularies']) ? $data['vocabularies'] : array(),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  if (!empty($data['node_types']) || !empty($data['vocabularies'])) {
    $form['markup'] = array(
      '#type' => 'markup',
      '#markup' => "</br></br>" . l(t("Run initial setup for selected nodes and vocabularies"), 'admin/config/drupal_sync/drupal_sync_initial_setup/run'),
    );
  }

  return $form;
}


/**
 * Form submit
 */
function drupal_sync_initial_setup_admin_settings_form_submit($form, $form_state) {
  $node_types = (isset($form_state['values']['node_types'])) ? $form_state['values']['node_types'] : 0;
  $vocabularies = isset($form_state['values']['vocabularies']) ? $form_state['values']['vocabularies'] : 0;

  // remove empty
  foreach ($node_types as $key => $value) {
    if (empty($value)) {
      unset($node_types[$key]);
    }
  }

  foreach ($vocabularies as $key => $value) {
    if (empty($value)) {
      unset($vocabularies[$key]);
    }
  }

  $data = array(
    'node_types' => $node_types,
    'vocabularies' => $vocabularies);
  variable_set('drupal_sync_initial_setup_settings', $data);
  drupal_set_message(t('The settings are saved.'), 'status');
}