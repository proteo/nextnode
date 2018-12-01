<?php

/**
 * @file
 * Ctools API related functions.
 */

/**
 * Implements hook_ctools_plugin_directory().
 */
function nextnode_ctools_plugin_directory($module, $plugin) {
  if ($module == 'ctools' && !empty($plugin)) {
    return 'plugins/' . $plugin;
  }
}
