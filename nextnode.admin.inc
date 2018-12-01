<?php

/**
 * @file
 * Description.
 */

/**
 * Page callback. Generate the Profile list.
 *
 * @return array
 *   Page render array.
 */
function nextnode_profile_list() {
  $header = array(
    t('Title'),
    t('Status'),
    t('View mode'),
    t('Fetch mode'),
    t('Assigned to'),
    t('Actions'),
  );
  $rows = array();
  $entity_info = entity_get_info('node');
  foreach (nextnode_load_profiles() as $profile) {
    $links_options = array('query' => drupal_get_destination());
    $status = $profile->active ? t('Active') : t('Inactive');
    if ($profile->fetch_mode == 'view') {
      list ($view_name, $view_display) = explode(':', $profile->view_name);
      $view = views_get_view($view_name);
      $link = l($view->human_name, 'admin/structure/views/view/' . $view_name . '/edit/' . $view_display);
      $fetch_mode = 'Views (' . $link . ')';
    }
    else {
      $fetch_mode = 'Basic';
    }
    $types = nodenext_profile_get_assigned_types($profile->name);
    if (!empty($types)) {
      $links = array();
      foreach ($types as $type => $name) {
        $links[] = l($name, 'admin/structure/types/manage/' . $type, $links_options);
      }
      $assigned_to = implode(', ', $links);
    }
    else {
      $assigned_to = htmlentities('<none>');
    }
    if ($profile->view_mode == 'default') {
      $view_mode = 'Default';
    }
    else {
      $view_mode = $entity_info['view modes'][$profile->view_mode]['label'];
    }
    $actions = array();
    foreach (array('edit', 'clone', 'delete') as $action) {
      $actions[] = l(t($action), 'admin/config/content/nextnode/' . $profile->pid . '/' . $action, $links_options);
    }
    $rows[] = array(
      'title' => $profile->title,
      'status' => $status,
      'view_mode' => $view_mode,
      'fetch_mode' => $fetch_mode,
      'assigned_to' => $assigned_to,
      'actions' => implode(' | ', $actions),
    );
  }
  $page = array();
  if (!empty($rows)) {
    $page['profiles']['#markup'] = theme('table', array('header' => $header, 'rows' => $rows));
  }
  return $page;
}

/**
 * Form constructor to add/edit NextNode profiles.
 */
function nextnode_profile_form($form, &$form_state, $profile = NULL) {
  if (empty($profile)) {
    $profile = nextnode_default_profile();
  }
  else {
    $title = t('<em>Edit profile</em> @title', array('@title' => $profile->title));
    drupal_set_title($title, PASS_THROUGH);
  }

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $profile->title,
    '#maxlength' => 128,
    '#required' => TRUE,
  );
  $form['name'] = array(
    '#type' => 'machine_name',
    '#description' => t('A unique machine-readable name for this profile. It must only contain lowercase letters, numbers, and underscores.'),
    '#maxlength' => 64,
    '#default_value' => $profile->name,
    '#required' => TRUE,
    '#machine_name' => array(
      'exists' => 'nextnode_profile_name_exists',
      'source' => array('title'),
    ),
  );
  $form['active'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#description' => t('Profile status.'),
    '#default_value' => $profile->active,
  );

  // Fetch options.
  $form['fetch'] = array(
    '#type' => 'fieldset',
    '#title' => t('Queue options'),
  );
  // Auto load.
  $form['fetch']['auto_trigger'] = array(
    '#type' => 'radios',
    '#title' => t('Auto load'),
    '#description' => t('When enabled, triggers the loading of the next set of content automatically when the user scrolls to the bottom of the containing element. When disabled, the required next link will trigger the loading of the next set of content when clicked.'),
    '#options' => array(
      1 => t('Enabled'),
      0 => t('Disabled'),
    ),
    '#default_value' => (int) $profile->auto_trigger,
    '#required' => TRUE,
  );
  // Auto load until...
  $form['fetch']['auto_trigger_until'] = array(
    '#type' => 'textfield',
    '#title' => t('Auto load until...'),
    '#description' => t('Enter an integer greater than 0 to turn off auto load of paging after the specified number of pages. Users will have to manually click the "Load more..." link to load more pages. Requires "Auto load" to be enabled.'),
    '#size' => 2,
    '#field_prefix' => 'Page &nbsp;',
    '#default_value' => $profile->auto_trigger_until,
    '#states' => array(
      'visible' => array(
        ':input[name="auto_trigger"]' => array('value' => 1),
      ),
    ),
  );
  // Max pages.
  $form['fetch']['max_pages'] = array(
    '#type' => 'textfield',
    '#title' => t('Page limit'),
    '#description' => t('Enter an integer greater than 0 to set a maximum number of pages (Ajax requests). Once that number is reached, no more nodes will be loaded (automatically or otherwise).'),
    '#size' => 2,
    '#field_suffix' => '&nbsp; pages',
    '#default_value' => $profile->max_pages,
  );
  // Fetch mode.
  $fetch_modes = array('basic' => 'Basic (time based sorting)');
  if (module_exists('views')) {
    $fetch_modes['view'] = t('Advanced (with Views)');
  }
  $form['fetch']['fetch_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Method'),
    '#description' => t('Defines the method used to select nodes delivered by NextNode in the Ajax request.'),
    '#options' => $fetch_modes,
    '#default_value' => $profile->fetch_mode,
    '#required' => TRUE,
  );
  // Number of results.
  $form['fetch']['num_results'] = array(
    '#type' => 'textfield',
    '#title' => t('Nodes per page'),
    '#description' => t('How many nodes will be returned on each page (Ajax request).'),
    '#size' => 3,
    '#default_value' => $profile->num_results,
    '#states' => array(
      'visible' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
      'required' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
    ),
  );
  // Content type filter.
  $types = node_type_get_names();
  $types = array_merge($types + array('__node__' => t("Matching current node's content type")));
  $form['fetch']['filter_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Filter by content type'),
    '#description' => t('Limit results to some content types. Leave unchecked to include all.'),
    '#options' => $types,
    '#default_value' => $profile->filter_types,
    '#states' => array(
      'visible' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
    ),
  );
  // Language filter.
  $form['fetch']['filter_language'] = array(
    '#type' => 'select',
    '#title' => t('Filter by language'),
    '#description' => t("Limit results using content's language."),
    '#options' => _nextnode_languages_options(),
    '#default_value' => $profile->filter_language,
    '#states' => array(
      'visible' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
    ),
  );
  // Sort criteria.
  $form['fetch']['order_by'] = array(
    '#type' => 'radios',
    '#title' => t('Order nodes by'),
    '#options' => array(
      'created' => t('Created timestamp'),
      'changed' => t('Changed timestamp'),
    ),
    '#default_value' => $profile->order_by,
    '#states' => array(
      'visible' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
      'required' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
    ),
  );
  // Sort order.
  $form['fetch']['sort_order'] = array(
    '#type' => 'radios',
    '#title' => t('Sort order'),
    '#options' => array(
      'DESC' => t('Descending'),
      'ASC' => t('Ascending'),
    ),
    '#default_value' => $profile->sort_order,
    '#states' => array(
      'visible' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
      'required' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
    ),
  );
  // Status filter.
  $form['fetch']['filter_status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only published content'),
    '#description' => t("Uncheck to include unpublished content in results. WARNING: this won't magically assign view permissions to unpublished nodes. Enable this only if you've taken steps to grant proper content access permissions or it WILL BREAK the queue."),
    '#default_value' => $profile->filter_status,
    '#states' => array(
      'visible' => array(
        ':input[name="fetch_mode"]' => array('value' => 'basic'),
      ),
    ),
  );
  // Give the chance to select a view only if the Views module is enabled.
  if (isset($fetch_modes['view'])) {
    $form['fetch']['view_name'] = array(
      '#type' => 'select',
      '#title' => t('View'),
      '#description' => t('Select the view used to fetch nodes. In this mode, the selected view will be in charge of filtering and sorting results.'),
      '#options' => _nextnode_views_options(),
      '#default_value' => $profile->view_name,
      '#states' => array(
        'visible' => array(
          ':input[name="fetch_mode"]' => array('value' => 'view'),
        ),
        'required' => array(
          ':input[name="fetch_mode"]' => array('value' => 'view'),
        ),
      ),
    );
  }
  // Anonymize.
  $form['fetch']['anonymize'] = array(
    '#type' => 'checkbox',
    '#title' => t('Fetch as anonymous'),
    '#description' => t('Check to run Ajax requests as anonymous user. Existent user sessions will remain intact, just content delivered by NextNode will be affected.'),
    '#default_value' => (int) $profile->anonymize,
  );

  // Node options.
  $form['node_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node options'),
    '#description' => t('Options for nodes delivered by NextNode.'),
  );
  // View mode.
  $form['node_options']['view_mode'] = array(
    '#type' => 'select',
    '#title' => t('View mode'),
    '#description' => t('Display view mode to render nodes delivered by NextNode.'),
    '#options' => _nextnode_view_modes_options(),
    '#default_value' => $profile->view_mode,
    '#required' => TRUE,
  );
  // Activate node title.
  $title_desc = "By default, most Drupal themes place the page title tag outside the node's markup, which may cause that nodes delivered by NextNode lack the title when using a view mode other than \"Teaser\". ";
  $title_desc .= "If that's the case, check this and use options below to add it back on render time. ";
  $title_desc .= "NOTE: don't activate this unless you need it or you may end up with duplicated titles. Also, this will NOT work if your nodes are being rendered using alternate methods like Panels.";
  $form['node_options']['add_node_title'] = array(
    '#type' => 'checkbox',
    '#title' => t('Add node title'),
    '#default_value' => $profile->add_node_title,
    '#description' => '<p>' . t($title_desc) . '</p>',
  );
  // Title element tag.
  $form['node_options']['title_element'] = array(
    '#type' => 'select',
    '#title' => t('Title tag'),
    '#description' => t('Tag used to generate title markup.'),
    '#options' => array(
      'h1' => '<h1>',
      'h2' => '<h2>',
      'h3' => '<h3>',
      'h4' => '<h4>',
      'h5' => '<h5>',
      'h6' => '<h6>',
      'div' => '<div>',
      'span' => '<span>',
    ),
    '#default_value' => $profile->title_element,
    '#states' => array(
      'visible' => array(
        ':input[name="add_node_title"]' => array('checked' => TRUE),
      ),
      'required' => array(
        ':input[name="add_node_title"]' => array('checked' => TRUE),
      ),
    ),
  );
  // Linked title.
  $form['node_options']['title_add_link'] = array(
    '#type' => 'checkbox',
    '#title' => t('Link title'),
    '#description' => t('Check to include a link to the node in the title.'),
    '#default_value' => $profile->title_add_link,
    '#states' => array(
      'visible' => array(
        ':input[name="add_node_title"]' => array('checked' => TRUE),
      ),
    ),
  );
  // Title element CSS.
  $form['node_options']['title_class'] = array(
    '#type' => 'textfield',
    '#title' => t('Title CSS class'),
    '#description' => t('Class(es) applied to the title element. Separate multiple class names using spaces.'),
    '#default_value' => $profile->title_class,
    '#maxlength' => 128,
    '#states' => array(
      'visible' => array(
        ':input[name="add_node_title"]' => array('checked' => TRUE),
      ),
    ),
  );
  // Title weight.
  $form['node_options']['title_weight'] = array(
    '#type' => 'textfield',
    '#title' => t('Title weight'),
    '#description' => t('This allows you to control the position of the title in the node. Lower values will move the title up in the node. The default value (-10) aims to put the title before anything else, but feel free to tweak it to fit your needs.'),
    '#size' => 3,
    '#default_value' => $profile->title_weight,
    '#states' => array(
      'visible' => array(
        ':input[name="add_node_title"]' => array('checked' => TRUE),
      ),
      'required' => array(
        ':input[name="add_node_title"]' => array('checked' => TRUE),
      ),
    ),
  );
  // Statistics Ajax counter.
  if (module_exists('statistics')
    && variable_get('statistics_count_content_views', 0)
    && variable_get('statistics_count_content_views_ajax', 0)) {
    $form['node_options']['count_stats'] = array(
      '#type' => 'checkbox',
      '#title' => t('Statistics Ajax support'),
      '#description' => t('Check to increase the Statistics Ajax counter when a new node is loaded.'),
      '#default_value' => $profile->count_stats,
    );
  }
  // Google Analytics page view tracking.
  $form['node_options']['ga_pageview'] = array(
    '#type' => 'checkbox',
    '#title' => t('Google Analytics support'),
    '#description' => t('Check to track Google Analytics page views when a new node is loaded.'),
    '#default_value' => $profile->ga_pageview,
  );
  // Inline Javascript.
  $desc = "Use this field to include inline Javascript code per node. The code itself will be included inside a &lt;script&gt; tag below every node returned in the Ajax request.<br>";
  $desc .= "Available placeholders for node values: [node-nid] (the numeric node ID), [node-type] (the content type) and [node-selector] (a convenience shortcut to be used as CSS selector, #nextnode-{nid}).<br>";
  $desc .= "NOTE: This should be your last resource to wire Javascript functionality to nodes delivered by NextNode. You're encouraged to write your code taking advantage of Drupal.behaviors and use proper, separate .js files. Check \"Companion assets\" below and see the README.txt file for a comprehensive explanation.";
  $form['node_options']['js_inline'] = array(
    '#type' => 'textarea',
    '#title' => t('Inline Javascript'),
    '#description' => t($desc),
    '#default_value' => $profile->js_inline,
  );
  // Inline Javascript wrapper.
  $form['node_options']['js_wrapper'] = array(
    '#type' => 'select',
    '#options' => array(
      'none' => t('Do not use wrapper'),
      'jquery' => t('Use jQuery wrapper'),
      'jquery_ready' => t('Use jQuery wrapper with document.ready handler'),
    ),
    '#default_value' => $profile->js_wrapper,
  );

  // Companion assets.
  $form['assets'] = array(
    '#type' => 'fieldset',
    '#title' => t('Companion assets'),
    '#description' => t('Inject CSS and JS files in pages of nodes (content types) assigned to this profile. NOTE: these files are included just once, when printing the page of the node issuing NextNode requests.'),
  );
  // CSS files.
  $form['assets']['css_files'] = array(
    '#type' => 'textarea',
    '#title' => t('CSS files'),
    '#description' => 'Enter one file per line. You can use "[module-{module_name}]" or "[theme-{theme_name}]" placeholders to get path to module/theme. You can use "[theme]" placeholder to get path to current page theme. Examples: "[theme]/css/styles.css" or "[theme-zen]/styles/main.css"',
    '#default_value' => $profile->css_files,
  );
  // JS files.
  $form['assets']['js_files'] = array(
    '#type' => 'textarea',
    '#title' => t('JS files'),
    '#description' => 'Enter one file per line. You can use "[module-{module_name}]" or "[theme-{theme_name}]" placeholders to get path to module/theme. You can use "[theme]" placeholder to get path to current page theme. Examples: "[theme]/js/scripts.js" or "[theme-zen]/scripts/main.js"',
    '#default_value' => $profile->js_files,
  );

  // Add information about the current profile if exists.
  if (isset($profile->pid) && !empty($profile->pid)) {
    $form['pid'] = array(
      '#type' => 'value',
      '#value' => $profile->pid,
    );
    $form['old_name'] = array(
      '#type' => 'value',
      '#value' => $profile->name,
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save changes'),
  );

  return $form;
}

/**
 * Validation callback for nextnode_profile_form.
 */
function nextnode_profile_form_validate($form, &$form_state) {
  $values = $form_state['values'];

  // "Limit number of pages" must be a numeric value.
  if ($values['max_pages'] !== '' && !is_numeric($values['max_pages'])) {
    form_set_error('max_pages', t('"Limit number of pages" must be a number.'));
  }
  switch ($values['fetch_mode']) {
    // Fields required in "Basic" fetch mode.
    case 'basic':
      // "Number of results" must be a numeric value.
      if (!is_numeric($values['num_results'])) {
        form_set_error('num_results', t('"Number of results" must be a number.'));
      }
      break;

    // Fields required in "Views" fetch mode.
    case 'view':
      // If for some reason there are no node-based views available...
      if (empty($values['view_name'])) {
        form_set_error('view_name', t('Select a valid view.'));
      }
      break;
  }
  // "Title weight" must be a numeric value.
  if (!empty($values['add_node_title']) && !is_numeric($values['title_weight'])) {
    form_set_error('title_weight', t('"Title weight" must be an number.'));
  }
  // "Auto trigger until" must be a numeric value.
  if ($values['auto_trigger_until'] !== '' && !is_numeric($values['auto_trigger_until'])) {
    form_set_error('auto_trigger_until', t('"Auto trigger until" must be a number.'));
  }
  // Validate CSS files.
  if (!empty($values['css_files'])) {
    $errors = _nextnode_validate_assets($values['css_files'], 'css');
    if (!empty($errors)) {
      $msg = t('CSS files:') . '<br>' . implode('<br>', $errors);
      form_set_error('css_files', $msg);
    }
  }
  // Validate JS files.
  if (!empty($values['js_files'])) {
    $errors = _nextnode_validate_assets($values['js_files'], 'js');
    if (!empty($errors)) {
      $msg = t('JS files:') . '<br>' . implode('<br>', $errors);
      form_set_error('js_files', $msg);
    }
  }
}

/**
 * Submit callback for nextnode_profile_form.
 */
function nextnode_profile_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  // This handy function will give us a $profile object filled with the
  // properties we need from form values.
  $profile = nextnode_default_profile($values);
  // Save profile data.
  $result = nextnode_profile_save($profile);

  switch ($result) {
    case SAVED_UPDATED:
      if (isset($values['old_name'])) {
        // If the name of the profile has been changed, we must update content
        // types that may be using it.
        if ($values['name'] != $values['old_name']) {
          _nextnode_update_profile_name($values['old_name'], $values['name']);
        }
      }
      break;

    case SAVED_NEW:
      $form_state['redirect'] = 'admin/config/content/nextnode';
      break;
  }
}

/**
 * Form constructor to delete NextNode profiles.
 */
function nextnode_profile_delete($form, &$form_state, $profile) {
  $form['pid'] = array(
    '#type' => 'value',
    '#value' => $profile->pid,
  );
  $form['profile_name'] = array(
    '#type' => 'value',
    '#value' => $profile->name,
  );
  $form['profile_title'] = array(
    '#type' => 'value',
    '#value' => $profile->title,
  );

  $question = t('Are you sure you want to delete the profile "@title"?', array('@title' => $profile->title));
  $description = t('This action cannot be undone.');
  $path = 'admin/config/content/nextnode';
  $yes = t('Delete');

  // Find content types assigned to this profile to include a warning message.
  $usage = array();
  foreach (nodenext_profile_get_assigned_types($profile->name) as $type => $label) {
    // Look if this content type is assigned to the profile.
    if (variable_get('nextnode_profile_' . $type) == $profile->name) {
      $usage[] = '<li>' . t('NextNode will be disabled on the content type %type', array('%type' => $label)) . '</li>';
    }
  }
  // Find nodes using this profile as override.
  $nodes = db_query('SELECT COUNT(nid) AS count FROM {nextnode_index} WHERE profile = :profile', array(':profile' => $profile->name))->fetchField();
  if ($nodes) {
    $usage[] = '<li>' . t('The profile will be deleted from %count nodes using it as override.', array('%count' => $nodes)) . '</li>';
  }

  if (!empty($usage)) {
    $message = t('If you proceed:');
    $description = $message . '<ul>' . implode('', $usage) . '</ul><br>' . $description;
  }

  return confirm_form($form, $question, $path, $description, $yes);
}

/**
 * Form constructor to clone a profile.
 */
function nextnode_profile_clone($form, &$form_state, $profile) {
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => '',
    '#maxlength' => 128,
    '#required' => TRUE,
  );
  $form['name'] = array(
    '#type' => 'machine_name',
    '#description' => t('A unique machine-readable name for this profile. It must only contain lowercase letters, numbers, and underscores.'),
    '#maxlength' => 64,
    '#default_value' => '',
    '#required' => TRUE,
    '#machine_name' => array(
      'exists' => 'nextnode_profile_name_exists',
      'source' => array('title'),
    ),
  );
  $form['pid'] = array(
    '#type' => 'value',
    '#value' => $profile->pid,
  );

  $question = t('Cloning profile "@title"', array('@title' => $profile->title));
  $path = 'admin/config/content/nextnode';
  $yes = t('Proceed');

  return confirm_form($form, $question, $path, '', $yes);
}

/**
 * Form validation handler for nextnode_profile_clone().
 */
function nextnode_profile_clone_validate($form, &$form_state) {
  $name = $form_state['values']['name'];
  $exists = nextnode_get_profile($name);
  if ($exists) {
    form_set_error('name', t('The name %name is already in use.', array('%name' => $name)));
  }
}

/**
 * Form submission handler for nextnode_profile_clone().
 */
function nextnode_profile_clone_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $profile = nextnode_profile_load($form_state['values']['pid']);
    // Remove the profile ID, because it belongs to the cloned profile.
    unset($profile->pid);
    // Set name and title.
    $profile->name = $form_state['values']['name'];
    $profile->title = $form_state['values']['title'];
    // Save the new profile.
    nextnode_profile_save($profile);
  }
}

/**
 * Form submission handler for nextnode_profile_delete().
 */
function nextnode_profile_delete_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $pid = $form_state['values']['pid'];
    $delete_name = $form_state['values']['profile_name'];
    $title = $form_state['values']['profile_title'];

    // Ensure this profile is no longer assigned to any content type.
    $types = nodenext_profile_get_assigned_types($delete_name);
    foreach (array_keys($types) as $type) {
      // Look if this content type is assigned to the profile.
      if (variable_get('nextnode_profile_' . $type) == $delete_name) {
        variable_del('nextnode_profile_' . $type);
        variable_del('nextnode_override_' . $type);
        // To clarify things, also disable NextNode in this content type, since
        // it no longer has a valid profile.
        variable_del('nextnode_enable_' . $type);
      }
    }

    // Now we can delete the db record.
    db_delete('nextnode_profiles')->condition('pid', $pid)->execute();

    // Delete from nodes using this profile as override.
    db_delete('nextnode_index')->condition('profile', $delete_name)->execute();

    // Invalidate the profiles cache.
    cache_clear_all('nextnode_profiles', 'cache');

    watchdog('nextnode', 'NextNode profile "@title" has been deleted.', array('@title' => $title));
    drupal_set_message(t('The profile "@title" has been deleted.', array('@title' => $title)));
  }
}

/**
 * NextNode global settings form constructor.
 */
function nextnode_settings_form($form, &$form_state) {
  // Check Waypoints library status.
  _nextnode_check_plugin_library();

  // General options fieldset.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
  );
  // Load offset.
  $form['options']['load_offset'] = array(
    '#type' => 'textfield',
    '#title' => t('Load offset'),
    '#description' => t('The distance from the bottom of the scrollable content at which to trigger the loading of the next set of content. This only applies when "Auto load" is active in the corresponding NextNode profile.'),
    '#default_value' => variable_get('nextnode_load_offset'),
    '#size' => 4,
    '#field_suffix' => 'px',
    '#required' => TRUE,
  );
  // Scrolling offset.
  $form['options']['scroll_offset'] = array(
    '#type' => 'textfield',
    '#title' => t('Scroll offset'),
    '#description' => t('The distance from node boundaries before triggering scrolling events (like page title changing).'),
    '#default_value' => variable_get('nextnode_scroll_offset'),
    '#size' => 4,
    '#field_suffix' => 'px',
    '#required' => TRUE,
  );
  // Change URL.
  $form['options']['change_url'] = array(
    '#type' => 'checkbox',
    '#title' => t('Update page title and URL'),
    '#description' => t('If checked, URL address and page title will change when scrolling over new content to reflect the values from the node active in the viewport.'),
    '#default_value' => variable_get('nextnode_change_url'),
  );
  // Loading CSS.
  $form['options']['load_css'] = array(
    '#type' => 'checkbox',
    '#title' => t('Include NextNode styling'),
    '#description' => t("Check to load NextNode's base style sheet. Disable this if you have added your own styling."),
    '#default_value' => variable_get('nextnode_load_css'),
  );
  // Loading CSS.
  $form['options']['minified_code'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use minified loader code'),
    '#description' => t('Check to use the minified version of NextNode Javascript files.'),
    '#default_value' => variable_get('nextnode_minified_code'),
  );

  // Sticky title.
  $form['sticky_title'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sticky title'),
  );
  // Enable sticky title.
  $form['sticky_title']['enable_sticky_title'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable sticky title'),
    '#description' => t('Display a floating element with the title of the node currently active in the viewport and a scroll progress indicator. The element itself is a standard block that you can place in any region of your theme.'),
    '#default_value' => variable_get('nextnode_sticky_title'),
  );
  // CSS z-index.
  $form['sticky_title']['sticky_title_zindex'] = array(
    '#type' => 'textfield',
    '#title' => t('Stacking position'),
    '#description' => t("CSS z-index value assigned to the floating title element. If your theme uses a fixed navbar you may need to adjust this. Leave empty if you prefer to control this property from your theme's CSS."),
    '#default_value' => variable_get('nextnode_sticky_title_zindex'),
    '#size' => 4,
    '#states' => array(
      'visible' => array(
        ':input[name="enable_sticky_title"]' => array('checked' => TRUE),
      ),
    ),
  );
  // CSS top position.
  $form['sticky_title']['sticky_title_top'] = array(
    '#type' => 'textfield',
    '#title' => t('Top space'),
    '#description' => t("Distance from the top of the viewport when the floating element is visible (CSS top position). If your theme uses a fixed navbar you may need to adjust this. Leave empty if you prefer to control this property from your theme's CSS."),
    '#default_value' => variable_get('nextnode_sticky_title_top'),
    '#size' => 4,
    '#field_suffix' => 'px',
    '#states' => array(
      'visible' => array(
        ':input[name="enable_sticky_title"]' => array('checked' => TRUE),
      ),
    ),
  );
  // Show offset.
  $form['sticky_title']['sticky_title_offset'] = array(
    '#type' => 'textfield',
    '#title' => t('Show/hide offset'),
    '#description' => t('Offset distance from the position of the element in the DOM (which is determined by the location of the block in your template) used to calculate when the floating title element should be shown/hidden. The default value (-100) aims to show the element after 100px have been scrolled past the position of the block. If the element is appearing too soon (or too late) modify this value to your preferences.'),
    '#default_value' => variable_get('nextnode_sticky_title_offset'),
    '#size' => 4,
    '#field_suffix' => 'px',
    '#required' => TRUE,
    '#states' => array(
      'visible' => array(
        ':input[name="enable_sticky_title"]' => array('checked' => TRUE),
      ),
    ),
  );

  // Selectors fieldset.
  $form['selectors'] = array(
    '#type' => 'fieldset',
    '#title' => t('Waypoints selectors'),
    '#description' => t("These options affect how Waypoints interacts with the module. Use them carefully and don't change anything unless you know what are you doing."),
  );
  // Main selector class name.
  $form['selectors']['hook_selector'] = array(
    '#type' => 'textfield',
    '#title' => t('Main selector class'),
    '#description' => t("Class name of the element used to initialize the plugin. You may need to change it if you've overridden the nextnode_wrapper theme function. Default value: nextnode-wrapper"),
    '#default_value' => variable_get('nextnode_hook_selector'),
    '#field_prefix' => '$(.',
    '#field_suffix' => ')',
    '#size' => 32,
    '#required' => TRUE,
  );
  // Next link selector class name.
  $form['selectors']['next_selector'] = array(
    '#type' => 'textfield',
    '#title' => t('Next selector class'),
    '#description' => t("The selector to use for finding the link which contains the href attribute pointing to the next set of content. You may need to change it if you've overridden the nextnode_link theme function. Default value: nextnode-next"),
    '#default_value' => variable_get('nextnode_next_selector'),
    '#field_prefix' => '$(.',
    '#field_suffix' => ')',
    '#size' => 32,
    '#required' => TRUE,
  );
  // Content selector class name.
  $form['selectors']['content_selector'] = array(
    '#type' => 'textfield',
    '#title' => t('Content selector class'),
    '#description' => t('This is a selector string that should match the individual items that will be pulled out of each page and appended to the item container. Default value: nextnode-node.'),
    '#default_value' => variable_get('nextnode_content_selector'),
    '#field_prefix' => '$(.',
    '#field_suffix' => ')',
    '#size' => 32,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save changes'),
  );

  return $form;
}

/**
 * Validation callback for nextnode_settings_form.
 */
function nextnode_settings_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  if (!is_numeric($values['load_offset'])) {
    form_set_error('load_offset', t('"Load offset" must be a number.'));
  }

  if (!empty($values['enable_sticky_title'])) {
    if ($values['sticky_title_zindex'] != '' && !is_numeric($values['sticky_title_zindex'])) {
      form_set_error('sticky_title_zindex', t('"Stacking position" must be a number.'));
    }
    if ($values['sticky_title_top'] != '' && !is_numeric($values['sticky_title_top'])) {
      form_set_error('sticky_title_top', t('"Top space" must be a number.'));
    }
    if (!is_numeric($values['sticky_title_offset'])) {
      form_set_error('sticky_title_offset', t('"Show/hide offset" must be a number.'));
    }
  }
}

/**
 * Submit callback for nextnode_settings_form.
 */
function nextnode_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];

  variable_set('nextnode_load_offset', trim($values['load_offset']));
  variable_set('nextnode_scroll_offset', trim($values['scroll_offset']));
  variable_set('nextnode_change_url', $values['change_url']);
  variable_set('nextnode_load_css', $values['load_css']);
  variable_set('nextnode_minified_code', $values['minified_code']);
  variable_set('nextnode_sticky_title', $values['enable_sticky_title']);
  variable_set('nextnode_sticky_title_zindex', $values['sticky_title_zindex']);
  variable_set('nextnode_sticky_title_top', $values['sticky_title_top']);
  variable_set('nextnode_sticky_title_offset', $values['sticky_title_offset']);
  variable_set('nextnode_hook_selector', trim($values['hook_selector']));
  variable_set('nextnode_next_selector', trim($values['next_selector']));
  variable_set('nextnode_content_selector', trim($values['content_selector']));

  drupal_set_message(t('The changes have been saved'));
}

/**
 * Helper function for hook_form_node_type_form_alter().
 *
 * @see nextnode_form_node_type_form_alter()
 */
function _nextnode_form_node_type_form_alter(&$form, $form_state) {
  $type = $form['#node_type']->type;
  $profiles = nextnode_profiles_options(TRUE);

  // NextNode fieldset.
  $form['nextnode'] = array(
    '#type' => 'fieldset',
    '#title' => 'NextNode',
    '#weight' => 10,
    '#group' => 'additional_settings',
    '#attached' => array(
      'js' => array(
        'vertical-tabs' => drupal_get_path('module', 'nextnode') . '/assets/nextnode_type_vertical_tabs.js',
      ),
    ),
  );
  // Profile selector.
  if (!empty($profiles)) {
    // Enable checkbox.
    $form['nextnode']['nextnode_enable'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable NextNode functionality for this content type'),
      '#default_value' => variable_get('nextnode_enable_' . $type, 0),
    );
    $form['nextnode']['nextnode_profile'] = array(
      '#type' => 'select',
      '#title' => t('Select a default profile'),
      '#options' => $profiles,
      '#default_value' => variable_get('nextnode_profile_' . $type, ''),
      '#required' => TRUE,
      '#states' => array(
        'visible' => array(
          'input[name="nextnode_enable"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['nextnode']['nextnode_override'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable profile overriding'),
      '#description' => t('Check to enable individual nodes to override the selected profile. A new field will be available in node edit forms for this content type allowing editors to choose a profile.'),
      '#default_value' => variable_get('nextnode_override_' . $type, 0),
      '#states' => array(
        'visible' => array(
          'input[name="nextnode_enable"]' => array('checked' => TRUE),
        ),
      ),
    );
  }
  else {
    $form['nextnode']['nextnode_profile'] = array(
      '#type' => 'markup',
      '#title' => t('Select a default profile'),
      '#markup' => t('No active profiles available'),
    );
  }
}

/**
 * Helper function for hook_form_node_form_alter().
 *
 * @see nextnode_form_node_form_alter()
 */
function _nextnode_form_node_form_alter(&$form, $form_state) {
  // The profile assigned by default to this content type.
  $default_profile_name = variable_get('nextnode_profile_' . $form['#bundle'], '');
  // Get options for active NextNode profiles.
  $profiles = nextnode_profiles_options(TRUE);
  // Remove the profile assigned by default from the options array. It wouldn't
  // make sense to reassign it.
  unset($profiles[$default_profile_name]);

  // If no other profiles are available, don't even bother. Just create values
  // to be used as flags in nextnode_set_node_profile().
  if (empty($profiles)) {
    $form['nextnode_override'] = array(
      '#type' => 'value',
      '#value' => FALSE,
    );
    $form['nextnode_profile'] = array(
      '#type' => 'value',
      '#value' => '',
    );
    return;
  }

  // Default values.
  $overriding = FALSE;
  $profile_name = $default_profile_name;
  $default_profile = nextnode_get_profile($default_profile_name);
  // Get the profile assigned to this specific node, if any.
  if (isset($form['#node']) && isset($form['#node']->nid)) {
    $profile = nextnode_get_node_profile($form['#node']->nid);
    if ($profile) {
      $profile_name = $profile->name;
      if ($profile_name != $default_profile_name) {
        $overriding = TRUE;
      }
    }
  }

  $form['nextnode'] = array(
    '#type' => 'fieldset',
    '#title' => t('NextNode profile'),
    '#access' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#weight' => 10,
    '#attached' => array(
      'js' => array(
        'vertical-tabs' => drupal_get_path('module', 'nextnode') . '/assets/nextnode_node_vertical_tabs.js',
      ),
    ),
  );
  $form['nextnode']['nextnode_override'] = array(
    '#type' => 'checkbox',
    '#title' => t('Override default profile (@profile)', array('@profile' => $default_profile->title)),
    '#default_value' => $overriding,
  );
  $form['nextnode']['nextnode_profile'] = array(
    '#type' => 'select',
    '#title' => t('Select another profile'),
    '#options' => $profiles,
    '#default_value' => $profile_name,
    '#states' => array(
      'visible' => array(
        '#edit-nextnode-override' => array('checked' => TRUE),
      ),
      'required' => array(
        '#edit-nextnode-override' => array('checked' => TRUE),
      ),
    ),
  );
}

/**
 * Update profile name in assigned content types.
 *
 * @param string $old_name
 *   Profile name currently assigned.
 * @param string $new_name
 *   New profile name to be assigned.
 */
function _nextnode_update_profile_name($old_name, $new_name) {
  $count = 0;
  $types = nodenext_profile_get_assigned_types($old_name);
  foreach ($types as $type => $name) {
    variable_set('nextnode_profile_' . $type, $new_name);
    $count++;
  }
  if ($count) {
    drupal_set_message(t('@count content types assigned to this profile have been updated.', array('@count' => $count)));
  }
}

/**
 * Available views for use as #options array.
 *
 * @return array
 *   An associative array for use in select.
 *   - key: view name and display id separated by ':'.
 */
function _nextnode_views_options() {
  $views = views_get_enabled_views();
  $options = array();
  foreach ($views as $view_name => $view) {
    // Only node based views.
    if ($view->base_table == 'node') {
      $displays = array();
      foreach ($view->display as $display_name => $display) {
        $key = $view_name . ':' . $display_name;
        $displays[$key] = $display->display_title;
      }
      $options[$view->human_name] = $displays;
    }
  }
  return $options;
}

/**
 * Available languages for use as #options array.
 *
 * @return array
 *   Enabled languages keyed by language code.
 */
function _nextnode_languages_options() {
  $languages = language_list('enabled');
  $languages = $languages[1];
  $list = array(
    'any' => t('- Any -'),
    'default' => t('Default site language'),
    'current' => t("Current user's language"),
    'node' => t("Matching current node's language"),
    LANGUAGE_NONE => t('Language neutral'),
  );
  foreach ($languages as $language) {
    $list[$language->language] = t($language->name);
  }
  return $list;
}

/**
 * Existing node view modes for use as #options array..
 *
 * @return array
 *   View modes keyed by type.
 */
function _nextnode_view_modes_options() {
  $entity_info = entity_get_info('node');
  $options = array('default' => 'Default');
  foreach ($entity_info['view modes'] as $view_mode => $info) {
    $options[$view_mode] = $info['label'];
  }
  return $options;
}

/**
 * Validates input from the CSS and JS files text fields.
 *
 * @param string $value
 *   Text field value.
 * @param string $ext
 *   Expected file extension (css or js).
 *
 * @return array
 *   Errors generated after validating if each line of the text represents a
 *   file that exists and has the expected extension.
 */
function _nextnode_validate_assets($value, $ext) {
  $errors = array();
  $default_theme = variable_get('theme_default', NULL);
  $theme_path = drupal_get_path('theme', $default_theme);
  $lines = array_filter(array_map('trim', explode(PHP_EOL, $value)), 'drupal_strlen');

  foreach ($lines as $line) {
    $file = str_replace('[theme]', $theme_path, $line);
    $matches = array();
    foreach (array('module', 'theme') as $type) {
      $pattern = '/\[' . $type . '-(.+)\]/';
      preg_match($pattern, $file, $matches);
      if (count($matches) > 1 && ($path = drupal_get_path($type, $matches[1])) != '') {
        $file = str_replace($matches[0], $path, $file);
      }
    }
    // Validate if file exists.
    if (!file_exists($file)) {
      $errors[] = t("The file %file doesn't exists.", array('%file' => $file));
    }
    else {
      // Validate file extension.
      $pattern = '/\.(' . $ext . ')$/i';
      if (!preg_match($pattern, $file)) {
        $errors[] = t("The file %file doesn't have the .%ext extension.", array('%file' => $file, '%ext' => $ext));
      }
    }
  }

  return $errors;
}

/**
 * Check Waypoints library status and set informative messages.
 */
function _nextnode_check_plugin_library() {
  if ($_SERVER['REQUEST_METHOD'] != 'GET' || drupal_get_messages(NULL, FALSE)) {
    // Don't run when a form is being submitted or we have other messages.
    return;
  }

  $library = libraries_detect('waypoints');
  $version = isset($library['version']) ? $library['version'] : NULL;
  $error_message = isset($library['error message']) ? $library['error message'] : '';

  if (empty($library['installed'])) {
    drupal_set_message(t('!error Please download the !link and see README.txt for installation instructions.', array(
      '!error' => $error_message,
      '!link' => l(t('Waypoints library'), $library['download url']),
    )), 'error');
  }
  elseif (version_compare($version, NEXTNODE_WAYPOINTS_MIN_VERSION, '>=')) {
    drupal_set_message(t('Waypoints library version @ver installed correctly.', array('@ver' => $version)));
  }
  else {
    drupal_set_message(t('The Waypoints library is outdated. You need to !link at least version @ver and replace the old version located in the %path directory on your server.', array(
      '!link' => l(t('download'), $library['download url']),
      '@ver' => NEXTNODE_WAYPOINTS_MIN_VERSION,
      '%path' => $library['library path'],
    )), 'error');
  }
}
