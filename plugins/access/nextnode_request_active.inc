<?php

/**
 * @file
 * Provides access control evaluating if the request was initiated by NextNode.
 */

$plugin = array(
  'title' => t('NextNode request active'),
  'description' => 'Evaluates if the active request was initiated by NextNode.',
  'callback' => 'nextnode_request_active_ctools_access_check',
  'summary' => 'nextnode_request_active_ctools_access_summary',
  'default' => array('negate' => 0),
);

/**
 * NextNode request active summary description callback.
 */
function nextnode_request_active_ctools_access_summary($conf, $context) {
  return t('NextNode request active');
}

/**
 * NextNode request active access check callback.
 */
function nextnode_request_active_ctools_access_check($conf, $context) {
  return (defined("NEXTNODE_REQUEST") && NEXTNODE_REQUEST);
}
