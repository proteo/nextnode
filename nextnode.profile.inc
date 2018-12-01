<?php

/**
 * @file
 * Functions related to profile handling.
 */

/**
 * Profile loader callback.
 *
 * @return object|bool
 *   NextNode profile object or FALSE if provided pid doesn't exist.
 */
function nextnode_profile_load($pid) {
  foreach (nextnode_load_profiles() as $name => $profile) {
    if ($profile->pid == $pid) {
      return $profile;
    }
  }
  return FALSE;
}

/**
 * Returns the profile with the given name.
 *
 * @param string $name
 *   Machine name of a NextNode profile.
 *
 * @return bool|object
 *   NextNode profile matching the provided name, FALSE if no match was found.
 */
function nextnode_get_profile($name) {
  foreach (nextnode_load_profiles() as $profile_name => $profile) {
    if ($profile_name == $name) {
      return $profile;
    }
  }
  return FALSE;
}

/**
 * Gets the profile assigned to a given content type.
 *
 * @param string $content_type
 *   A valid node content type.
 *
 * @return object|bool
 *   A NextNode profile assigned to the given content type, or FALSE if none.
 */
function nextnode_get_content_type_profile($content_type) {
  $enabled = variable_get('nextnode_enable_' . $content_type);
  if ($enabled) {
    $profile_name = variable_get('nextnode_profile_' . $content_type);
    $profiles = nextnode_load_profiles();
    if (isset($profiles[$profile_name]) && $profiles[$profile_name]->active) {
      return $profiles[$profile_name];
    }
  }
  return FALSE;
}

/**
 * Load the profile assigned to the node issuing the Ajax request.
 *
 * @return object|bool
 *   A NextNode profile if the content type of the given node has one assigned,
 *   FALSE otherwise.
 */
function nextnode_get_active_profile() {
  if (defined('NEXTNODE_REQUEST_NID')) {
    // First check if this specific node is overriding its default profile.
    $profile = nextnode_get_node_profile(NEXTNODE_REQUEST_NID);
    if (!$profile) {
      // Use the profile assigned by default to the content type.
      $type = nextnode_get_node_value(NEXTNODE_REQUEST_NID, 'type');
      $profile = nextnode_get_content_type_profile($type);
    }
    if ($profile && $profile->active) {
      return $profile;
    }
  }

  return FALSE;
}

/**
 * Load NextNode profiles from the database.
 *
 * @return array
 *   Associative array with existent NextNode profiles keyed by name.
 */
function nextnode_load_profiles() {
  $profiles = &drupal_static(__FUNCTION__);
  if (!isset($profiles)) {
    $cache = cache_get('nextnode_profiles');
    if ($cache) {
      $profiles = $cache->data;
    }
    else {
      $profiles = array();
      $result = db_select('nextnode_profiles', 'p')
        ->fields('p')
        ->orderBy('pid', 'DESC')
        ->execute();
      foreach ($result as $row) {
        $profile = array(
          'pid' => $row->pid,
          'name' => $row->name,
          'title' => $row->title,
          'active' => $row->active,
        );
        $config = unserialize($row->config);
        foreach ($config as $name => $value) {
          $profile[$name] = $value;
        }
        $profiles[$row->name] = nextnode_default_profile($profile);
      }
      cache_set('nextnode_profiles', $profiles, 'cache');
    }
  }
  return $profiles;
}

/**
 * Return content types assigned to a given NextNode profile.
 *
 * @param string $name
 *   A profile machine name.
 * @param bool $enabled_only
 *   Flag to discriminate content types that are assigned to the profile but
 *   don't have NextNode functionality enabled.
 *
 * @return array
 *   Associative array with names keyed by type.
 */
function nodenext_profile_get_assigned_types($name, $enabled_only = FALSE) {
  $types = array();
  foreach (node_type_get_types() as $type) {
    // Find the assigned profile.
    $profile = variable_get('nextnode_profile_' . $type->type, FALSE);
    if ($profile && $profile == $name) {
      // Has this content type NextNode functionality enabled?
      $enabled = variable_get('nextnode_enable_' . $type->type, FALSE);
      if (!$enabled_only || $enabled) {
        $types[$type->type] = $type->name;
      }
    }
  }
  return $types;
}

/**
 * Stores a NextNode profile into the database.
 *
 * @param object $profile
 *   A NextNode profile object.
 * @param bool $verbose
 *   Flag to display informative messages.
 *
 * @return bool|int
 *   Result of drupal_write_record().
 */
function nextnode_profile_save($profile, $verbose = TRUE) {
  // Create profile record.
  $record = new stdClass();
  // Save the ID if this is an existen profile.
  if (isset($profile->pid) && !empty($profile->pid)) {
    $record->pid = $profile->pid;
    unset($profile->pid);
  }
  // Set values for the main db columns.
  $record->name = $profile->name;
  unset($profile->name);
  $record->title = $profile->title;
  unset($profile->title);
  $record->active = (bool) $profile->active;
  unset($profile->active);
  // Any other value passed in the profile will be stored in the config array.
  $record->config = array();
  foreach ($profile as $name => $value) {
    $record->config[$name] = $value;
  }

  if (isset($record->pid)) {
    // Update record.
    $result = drupal_write_record('nextnode_profiles', $record, array('pid'));
    if ($verbose) {
      drupal_set_message(t('The profile "@title" has been updated.', array('@title' => $record->title)));
    }
  }
  else {
    // Create record.
    $result = drupal_write_record('nextnode_profiles', $record);
    if ($verbose) {
      drupal_set_message(t('The profile "@title" has been created.', array('@title' => $record->title)));
    }
  }

  // Invalidate the profiles cache.
  cache_clear_all('nextnode_profiles', 'cache');

  return $result;
}

/**
 * Checks if a NextNode profile exists.
 *
 * @param string $name
 *   Machine name of the profile.
 *
 * @return bool
 *   TRUE if an existent profile matches the given name, FALSE otherwise.
 */
function nextnode_profile_name_exists($name) {
  $profiles = nextnode_load_profiles();
  return isset($profiles[$name]);
}

/**
 * Provides a NextNode profile object.
 *
 * This functions returns a "canonical" profile with every expected property
 * filled with the provided value or a default one (if no value is passed).
 *
 * @param array $data
 *   Associative array with profile values.
 *
 * @return \stdClass
 *   Object with the following properties set to its default values:
 *   - name: Profile's machine name.
 *   - title: Profile's human-readable name.
 *   - active: Boolean value used to determine if a profile is in active status.
 *   - view_mode: The display view mode used to render nodes.
 *   - auto_trigger: Activates the auto loading option. When enabled, triggers
 *     the loading of the next set of content automatically when the user
 *     scrolls to the bottom of the containing element.
 *   - auto_trigger_until: If an integer greater than 1 is set, it will turn off
 *     autoloading after the specified number of pages has been reached.
 *   - max_pages: Limits the maximum possible number of requests. Used to stop
 *     lists to become "infinite".
 *   - fetch_mode: Method to retrieve nodes when NextNode process a request.
 *     - basic: Simple, timestamp-based sorting. No fancy options here.
 *     - view: Uses a view to define complex filter and sort criteria.
 *   - filter_types: Array with allowed content types when fetching nodes.
 *     This property is used when operating in 'basic' mode only.
 *   - filter_language: Language (code) used to fetch nodes. This property is
 *     used when operating in 'basic' mode only.
 *   - filter_status: Published status used to fetch nodes. This property is
 *     used when operating in 'basic' mode only.
 *   - order_by: Order criteria. Possible values are 'created' to sort nodes by
 *     creation date or 'changed' to sort by modification time. This property is
 *     used when operating in 'basic' mode only.
 *   - sort_order: Sorting order, possible values are DESC or ASC. This property
 *     is used when operating in 'basic' mode only.
 *   - num_results: Number of nodes to be fetched. This property is used when
 *     operating in 'basic' mode only.
 *   - view_name: Name and display of the view used to filter/sort nodes. This
 *     property is used when operating in 'view' mode only
 *   - add_node_title: When checked, NextNode will add a new entry called
 *     "nextnode_title" to every node content array delivered using Ajax, which
 *     will be printed in the template.
 *   - title_add_link: When "add_node_title" is active, used to convert the
 *     title into a link pointing to the node.
 *   - title_element: When "add_node_title" is active, defines the HTML tag used
 *     to create the title.
 *   - title_class: When "add_node_title" is active, class name(s) added to the
 *     title HTML element.
 *   - title_weight: When "add_node_title" is active, defines the order of the
 *     title element in the node.
 *   - anonymize: When active, NextNode will switch to the anonymous user before
 *     fetching and rendering nodes.
 *   - count_stats: When active, NextNode will perform additional steps to
 *     ensure that Ajax delivered nodes will increase its statistics counter.
 *     This option will be available only if the Statistics module is enabled.
 *   - css_files: Array of CSS files to be loaded when delivering nodes.
 *   - css_inline: Inline CSS code to be loaded when delivering nodes.
 *   - js_files: Array of Javascript files to be loaded when delivering nodes.
 *   - js_inline: Inline Javascript code to be loaded when delivering nodes.
 *   - js_wrapper: When inline JS code is being loaded, this defines a wrapper
 *     for that code:
 *     - none: No wrapper.
 *     - jquery: Adds a jQuery wrapper.
 *     - jquery_ready: Adds a jQuery wrapper with document.ready handler.
 */
function nextnode_default_profile(array $data = array()) {
  $profile = new stdClass();
  if (isset($data['pid']) && intval($data['pid']) > 0) {
    $profile->pid = (int) $data['pid'];
  }
  $profile->name = isset($data['name']) ? trim($data['name']) : '';
  $profile->title = isset($data['title']) ? trim($data['title']) : '';
  $profile->active = isset($data['active']) ? (int) $data['active'] : 1;
  $profile->view_mode = isset($data['view_mode']) ? $data['view_mode'] : 'default';
  $profile->auto_trigger = isset($data['auto_trigger']) ? (bool) $data['auto_trigger'] : TRUE;
  $profile->auto_trigger_until = isset($data['auto_trigger_until']) ? (int) $data['auto_trigger_until'] : 0;
  $profile->max_pages = isset($data['max_pages']) ? (int) $data['max_pages'] : 0;
  $profile->fetch_mode = isset($data['fetch_mode']) ? $data['fetch_mode'] : 'basic';
  $profile->filter_types = isset($data['filter_types']) ? $data['filter_types'] : array();
  $profile->filter_language = isset($data['filter_language']) ? $data['filter_language'] : 'any';
  $profile->filter_status = isset($data['filter_status']) ? (int) $data['filter_status'] : 1;
  $profile->order_by = isset($data['order_by']) ? $data['order_by'] : 'created';
  $profile->sort_order = isset($data['sort_order']) ? $data['sort_order'] : 'DESC';
  $profile->num_results = isset($data['num_results']) ? (int) $data['num_results'] : 1;
  $profile->view_name = isset($data['view_name']) ? $data['view_name'] : 'nextnode:default';
  $profile->add_node_title = isset($data['add_node_title']) ? (bool) $data['add_node_title'] : FALSE;
  $profile->title_add_link = isset($data['title_add_link']) ? (bool) $data['title_add_link'] : FALSE;
  $profile->title_element = isset($data['title_element']) ? $data['title_element'] : 'h1';
  $profile->title_class = isset($data['title_class']) ? $data['title_class'] : 'page-header';
  $profile->title_weight = isset($data['title_weight']) ? $data['title_weight'] : '-10';
  $profile->anonymize = isset($data['anonymize']) ? (int) $data['anonymize'] : 1;
  $profile->count_stats = isset($data['count_stats']) ? (bool) $data['count_stats'] : FALSE;
  $profile->ga_pageview = isset($data['ga_pageview']) ? (bool) $data['ga_pageview'] : FALSE;
  $profile->css_files = isset($data['css_files']) ? trim($data['css_files']) : '';
  $profile->js_files = isset($data['js_files']) ? trim($data['js_files']) : '';
  $profile->js_inline = isset($data['js_inline']) ? trim($data['js_inline']) : '';
  $profile->js_wrapper = isset($data['js_wrapper']) ? $data['js_wrapper'] : 'none';

  return $profile;
}

/**
 * NextNode profiles for use as #options array.
 *
 * @param bool $active_only
 *   Set to TRUE to discriminate non-active profiles from the returning array.
 *
 * @return array
 *   Associative array keyed by profile name.
 */
function nextnode_profiles_options($active_only = FALSE) {
  $options = array();
  foreach (nextnode_load_profiles() as $name => $profile) {
    if (!$active_only || $profile->active) {
      $options[$name] = $profile->title;
    }
  }
  return $options;
}
