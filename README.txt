CONTENTS OF THIS FILE
---------------------

* Introduction
* Core features
* Requirements
* Installation
* Usage and configuration
  - Core concepts
  - How queues work
  - Profile configuration
  - Global configuration
  - What about Javascript?
* Frequently Asked Questions (FAQ)
* Known Issues
* Maintainers


INTRODUCTION
------------

NextNode provides functionality to load additional content "on the fly" on page
scrolling, an increasingly popular UX pattern. It uses the popular Waypoints
Javascript library to append Ajax-loaded content to node pages when users scroll
down the page and reach a given threshold.


CORE FEATURES
-------------

* Append content to existent node pages using Ajax when users reach the end
  of the page. In this way, your users can keep reading new content as they
  scroll down the page, whithout requiring any interaction from them (this
  default behavior can be modified).
* Optionally add a fixed element containing the title of the currently active
  node in the viewport, plus a percent meter indicator, for an optimal
  visual feedback as users scroll down the page.
* Administrators can create any number of "Profiles", which are customizable
  presets with specific settings to generate, filter and sort the loaded
  content. In this way, you can control how many nodes will be fetched with
  each request, its order, content type, format, etc. Each profile can be
  assigned by content type.
* Support for nodes rendered using Page Manager/Panels.
* Compatible with Drupal's page cache and Boost module.
* Compatible with Drupal's core statistics counter (using Ajax) and Google
  Analytics in order to register view stats for nodes served by NextNode.


REQUIREMENTS
------------

- Drupal version greater than or equal to 7.17.
- Libraries 2.x contributed module.
- Waypoints Javascript library.
- Having clean URLs enabled is higly recommended, since all development and
  testing of the module has been done in this way. See:
  https://www.drupal.org/docs/7/configuring-clean-urls/enable-clean-urls


INSTALLATION
------------

1. Download the latest version of the Waypoints library from the following link:
   https://github.com/imakewebthings/waypoints/zipball/latest
2. Unpack the contents of the zip file. Look for the folder called "lib", rename
   it and move it to your Drupal installation so it becomes available at the
   following path: sites/all/libraries/waypoints
   So the final structure of the libraries directory should end up like this:
   - libraries/
     - waypoints/
       - shortcuts/
       - jquery.waypoints.js
       - jquery.waypoints.min.js
       - noframework.waypoints.js
       - noframework.waypoints.min.js
       - waypoints.debug.js
       - zepto.waypoints.js
       - zepto.waypoints.min.js
3. Install this module and the Libraries 2.x module as you would normally
   install any contributed Drupal module. For further information, see:
   https://drupal.org/documentation/install/modules-themes/modules-7
4. Once you're done, you may want to point your browser at /admin/reports/status
   and verify that Drupal recognizes the Waypoints library properly.


USAGE AND CONFIGURATION
-----------------------

CORE CONCEPTS

As already mentioned, the basic purpose of this module is to append Ajax-loaded
content to your node pages (and only node pages). There are a few concepts
involved in the process which you'll need to be familiarized with:

- Parent node: is the node that initiates the process, in other words, the node
  already present in the page before any scrolling is performed.
- NextNode request: the Ajax-powered request/response cycle performed in order
  to fetch the nodes that will be appended to the page as users scroll down.
- Queue: is the *virtual* list of nodes that will be appended to the page of the
  parent node. The queue is therefore created from other nodes wich will be
  filtered, sorted and formatted according to a specific, configurable criteria.
- NextNode profile: is an user-configurable preset that holds the settings used
  to customize NextNode requests and defines the characteristics of its response.

To summarize how the user-facing functionality of the module works:

1. The user visits any node page (this is what we'll call the parent node).
2. The user scrolls down the page, reaching a threshold that fires the request.
3. The request is performed and fetches a node (or nodes) from the server. How
   many and which nodes are fetched depends on the configuration of the profile
   assigned to the content type of the parent node.
4. The new content (the fetched node or nodes) is appended to the page below
   the parent node.
5. The user scrolls down the page, and the process starts again. Note that you
   can also customize this behavior from the corresponding profile, limiting the
   requests that can be performed, or even requiring users to click a button in
   order to fetch new nodes.

At the admin level, the process is basically:

1. Create a new profile, and configure it to your heart's content.
2. Assign the profile to your node's content type. Profiles are reusable, so you
   can assign the same profile to as many content types as you wish.

HOW QUEUES WORK

As we mentioned, profiles are the entities controlling how the request/response
cycle works, and a crucial part of it is the queue, or list of of nodes to be
delivered by each request. Each profile has two methods to define how many and
which nodes will be selected by the queue:

1. Basic (time based sorting). This method uses standard node properties (content
   type, language, created timestamp, etc.) to create a simple filtering and
   sorting criteria.
2. Advanced (available if the Views module is enabled). It allows you to use the
   view of your choosing to create complex filtering and sorting criteria. You
   can use any number of mechanisms, contrib modules, etc. as long as they are
   compatible with Views in order to define such criteria. In this mode it is
   specially important to exclude the parent node from appearing in the result
   set, which would create a duplicate (or even break the page). Please check
   the preconfigured view called "NextNode queue", which is provided by the
   module and can be used as start point or template to create your own.
   Note tha the view is used only to filter and sort the queue –in other words,
   to figure out what node or nodes will be presented next– but not to format
   the output, which is defined by the view mode selected in the "Node options"
   area of the active profile settings.

PROFILE CONFIGURATION

1. Point your browser at:
   http://[yourdomain]/admin/config/content/nextnode
2. Create a new profile using the "Add profile" button or use the default one
   that is provided with the module, clicking on the corresponding "edit" link.
3. Configure the profile to match your preferences. Use the "Method" radios to
   define the method used to build the queue. If you select the "Advanced" mode,
   make sure to select the corresponding view and to configure it according to
   your needs.
4. Save the changes to the profile.
5. Open the configuration screen of any content type, locate the "NextNode" tab
   and check the "Enable NextNode functionality for this content type" box. You
   will need to select the profile you just created or edited. Now, every time
   a node of this type is opened, the settings of the selected profile will be
   used to fetch nodes when users scroll down the page. Note that your editors
   will be able to override the selected profile and choose a different one on
   node basis if you also check the "Enable profile overriding" checkbox.

GLOBAL CONFIGURATION

Besides per-profile configuration, you can modify the global functionality of
the module at the following location:

http://[yourdomain]/admin/config/content/nextnode/settings

The "Sticky title" option, which aims to improve visual feedback, deserves a
special mention. Enable this option to add a floating element at the top of the
page which will display the title of the node currently active in the viewport
and a progress meter that is updated as the user scrolls down the page.

WHAT ABOUT JAVASCRIPT?

Unless your site is a very basic one, you may need Javascript to act on nodes
delivered via the Ajax request. There are two ways to do this:

1. Using inline Javascript.
   Intended for simple scenarios, when all you need is to perform some
   action as nodes are being inserted into the DOM. Under the "Node options"
   fieldset of your active profile there is a text area where you can put
   Javascript code to be included as a inline <script> element with every node
   delivered in the request. There are some placeholders available that you
   can use in the code to address the node currently being inserted. Please note
   that this being inline code, it is executed as soon as the node is inserted
   in the DOM, and before other events (see below) are fired. Depending on
   your code's complexity, it may be a bit tricky to make it work.

2. Using custom events.
   NextNode will fire some custom events that you can catch using Drupal
   Behaviors, providing a more robust and reliable mechanism to interact with
   the module. Write your code into a separate .js file and load it using the
   "Companion assets" option available in the profile configuration screen or
   your preferred method (from your theme, a custom module, etc.). A very good
   explanation of the options available is located at:
   https://www.drupal.org/docs/7/api/javascript-api/managing-javascript-in-drupal-7

   Note that even if the request may return several nodes, events are fired on
   each individual node. Available events are:

   nextnode.load
   Triggered after the node has been inserted into the DOM and custom events
   used by NextNode have been added.

   nextnode.firstscrollin
   Triggered when the user scrolls into the node for the first time (and
   therefore is fired only once).

   nextnode.scrollin
   Triggered every time the node enters into the viewport.

   nextnode.scrollout
   Triggered every time the node leaves the viewport.

   You can use this code snippet as a starting point for your own script:

   (function ($) {
   Drupal.behaviors.MyModule = {
     attach: function (context, settings) {
       // Use the .nextnode-node CSS selector to address individual nodes.
       $('.nextnode-node').once('my-module', function() {
         var $node = $(this);
         $node.on('nextnode.load', function(event) {
           console.log('This node was added to the DOM.');
         });
         $node.on('nextnode.firstscrollin', function(event) {
           console.log('This node is entering into the viewport for the first time.');
         });
         $node.on('nextnode.scrollin', function(event) {
           console.log('This node is entering into the viewport.');
         });
         $node.on('nextnode.scrollout', function(event) {
           console.log('This node is leaving the viewport.');
         });
       });
     }
   };
   })(jQuery);

   The expert eye will notice that I'm not using the context variable as
   argument for the selector function, as recommended by Drupal best practices.
   That's because when this code is invoked after receiving the response from
   a request, the context variable is the node element itself, so using it as
   function argument will cause the selector to fail.


FREQUENTLY ASKED QUESTIONS (FAQ)
--------------------------------

* Why nodes delivered by NextNode don't include the title?

  By default, most Drupal themes include page title markup in the page.tpl.php
  template, which is not rendered as part of NextNode requests (NextNode will
  strip any page elements other than the node itself). To fix this, check the
  "Add node title" option located under the "Node options" area of your active
  profile configuration form. Note: if you are using Page Manager/Panels and
  face this issue, checking this option will NOT work. In such case, make sure
  to add the page title using one of the panes available for that effect.

* I have enabled the "Sticky title" option, but I can't see it.

  Since the element is positioned with CSS (position:fixed), you may need to
  make adjustments to its properties (z-index, spacing) in order to make it work
  as intended, specially if your theme already displays other elements in the
  same screen area (such as toolbars, navigation menus, etc.). Use the options
  provided in the global configuration screen ("Stacking position" and "Top
  space") to re-position the element, or leave them empty and apply custom
  styling from your theme's CSS using the .nextnodesticky--active selector.

* I need to interact with the nodes delivered by NextNode using Javascript.

  Please refer to the "WHAT ABOUT JAVASCRIPT?" chapter above for a comprehensive
  explanation about the subject.


KNOWN ISSUES
------------

* The page is broken if the same node issuing the request is included in a
  response (the response contains a duplicate of the parent node).

  This can occur if you are using the "Advanced" (Views) queue method an have
  selected a view mode other than "Default" in the "Node options" area of your
  active profile. NextNode will try to replace the existent node element with
  the one received in the request, and doing so will break page structure.
  To avoid this, ensure that your view is using a contextual filter to exclude
  the parent node from the result set. You can do it providing a default value
  using PHP code returning the NEXTNODE_REQUEST_NID constant (which is defined
  by the module when serving the request). Please check the default view
  provided by the module, called "NextNode queue", to see a working example.


MAINTAINERS
-----------

Current maintainers:

* Original Author: Proteo <https://www.drupal.org/user/2644289>
