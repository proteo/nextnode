<?php

/**
 * @file
 * Plugin to provide access control based upon the active NextNode profile.
 *
 * Note that the access check will return FALSE if the current request was NOT
 * initiated by NextNode, because there is no active profile in such case.
 */

$plugin = array(
  'title' => t('NextNode active profile'),
  'description' => 'Evaluates the profile active in a NextNode request.',
  'callback' => 'nextnode_active_profile_ctools_access_check',
  'summary' => 'nextnode_active_profile_ctools_access_summary',
  'settings form' => 'nextnode_active_profile_ctools_access_settings',
  'default' => array('negate' => 0),
);

/**
 * NextNode active profile summary description callback.
 */
function nextnode_active_profile_ctools_access_summary($conf, $context) {
  if (isset($conf['profile'])) {
    $profile = nextnode_get_profile($conf['profile']);
    $output = t('Active NextNode profile is "@profile"', array('@profile' => $profile->title));
  }
  else {
    $output = t('No valid NextNode profile selected');
  }
  return $output;
}

/**
 * NextNode active profile settings form callback.
 */
function nextnode_active_profile_ctools_access_settings($form, &$form_state, $conf) {
  $form['settings']['profile'] = array(
    '#type' => 'select',
    '#title' => t('Select a profile'),
    '#options' => nextnode_profiles_options(TRUE),
    '#default_value' => $conf['profile'],
    '#required' => TRUE,
  );
  return $form;
}

/**
 * NextNode active profile access check callback.
 */
function nextnode_active_profile_ctools_access_check($conf, $context) {
  $profile = nextnode_get_active_profile();
  if ($profile && ($profile->name == $conf['profile'])) {
    return TRUE;
  }
  return FALSE;
}
