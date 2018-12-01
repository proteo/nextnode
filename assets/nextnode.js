/**
 * @file
 * NextNode loader script.
 */

(function($) {

Drupal.nextNode = {
  // Default options.
  defaults: {
    hookSelector: '.nextnode-wrapper',
    nextSelector: '.nextnode-next',
    contentSelector: '.nextnode-node',
    loadOffset: 200,
    scrollOffset: 100,
    autoTrigger: true,
    autoTriggerUntil: null,
    updatePageInfo: true,
    stickyTitle: true,
    stickyZindex: null,
    stickyTop: null,
    stickyOffset: -100,
    statsURL: null,
    countStatistics: false,
    gaTrackPageView: false
  },
  // Stores NextNode settings.
  options: {},
  // Stores original settings passed to the constructor.
  settings: {},
  // The context we're operating on, straight from the attached behavior.
  context: null,
  // Parent element used to initialize NextNode.
  element: null,
  // Loading link element.
  loadingElement: null,
  // Page title element.
  titleElement: null,
  // Sticky element.
  stickyElement: null,
  // Sticky title element.
  stickyTitleElement: null,
  // Sticky meter element.
  stickyMeterElement: null,
  // Our instance of Waypoint's Infinite shortcut (used to create the queue).
  infinite: null,
  // Stores the nid of the node active in the viewport.
  activeNode: null,
  // Stores the current page.
  currentPage: 0,
  // Initializes NextNode with the given settings.
  init: function(element, context, settings) {
    this.options = $.extend({}, this.defaults, settings.NextNode);
    this.settings = settings;
    this.context = context;
    this.element = element;
    this.loadingElement = $('#nextnode-loading', context);
    this.titleElement = $('title', context);
    // Create the main waypoint.
    this.initWaypoint();
    // Enable the sticky title.
    if (this.options.stickyTitle) {
      this.initSticky();
    }
    // Bind scroll events to the original node in the page.
    this.initScroll($(this.options.contentSelector, context));
  },
  // Creates the main waypoint using Waypoint's Infinite shortcut.
  initWaypoint: function() {
    var nn = this;
    this.infinite = new Waypoint.Infinite({
      element: nn.element,
      items: nn.options.contentSelector,
      more: nn.options.nextSelector,
      // Callback invoked before loading new content.
      onBeforePageLoad: function() {
        nn.loadingElement.show();
      },
      // Callback invoked after loading new content.
      onAfterPageLoad: function($items) {
        nn.loadingElement.hide();
        $(nn.options.nextSelector, nn.context).hide();
        // Initialize nodes.
        nn.initNodes($items);
        // Increase page counter.
        nn.currentPage++;
        // Evaluate if auto loading must be kept enabled or not. Check if auto
        // load is disabled or "auto load until" is enabled and the maximum
        // amount of automatic pages has been reached.
        if (!nn.options.autoTrigger || (nn.options.autoTriggerUntil !== null && nn.currentPage >= nn.options.autoTriggerUntil)) {
          // From now on automatic loading will be disabled. We disable it by
          // shutting down Infinite's waypoint to prevent firing any additional
          // load events.
          nn.infinite.waypoint.disable();
          // Bind the loading event to the current loader link so users can
          // trigger it manually.
          nn.loadOnClick();
        }
      },
      offset: function() {
        // Calculates the distance from the bottom edge of the node that
        // triggers the load of the next page.
        return (this.context.innerHeight() + nn.options.loadOffset) - this.adapter.outerHeight();
      },
    });
    // Check the auto loading setting.
    if (this.options.autoTrigger) {
      // If auto loading is enabled, all we have to do is to hide the link.
      $(this.options.nextSelector, this.context).hide();
    } else {
      // Disable the waypoint and enable manual loading on the link.
      this.infinite.waypoint.disable();
      this.loadOnClick();
    }
  },
  // Initializes nodes delivered by NextNode.
  initNodes: function($items) {
    var nn = this;
    $items.each(function() {
      var $node = $(this);
      // Create scroll events.
      nn.initScroll($node);
      // Bind our own first scroll-in event to update node statistics.
      $node.on('nextnode.firstscrollin', function(event) {
        // Increase node stats counter.
        if (nn.options.countStatistics && nn.options.statsURL !== null) {
          nn.updateStats($node);
        }
        // Track Google Analytics page views.
        if (nn.options.gaTrackPageView && typeof (ga) === 'function') {
          nn.trackPageView($node);
        }
      });
      // Attach Drupal behaviors.
      Drupal.attachBehaviors(this, nn.settings);
      // Trigger the load event of this node.
      $node.trigger('nextnode.load');
    });
  },
  // Creates scroll events.
  initScroll: function($node) {
    var nn = this;
    var Inview = new Waypoint.Inview({
      element: $node[0],
      enter: function(direction) {
        if (direction === 'down') {
          // Trigger (and turn off) the first scroll-in event.
          $node.trigger('nextnode.firstscrollin').off('nextnode.firstscrollin');
        }
        if (direction === 'up') {
          nn.activeNode = $node.data('nid');
          nn.updatePageInfo($node);
        }
        // Trigger the regular scroll-in event.
        $node.trigger('nextnode.scrollin', direction);
      },
      exit: function(direction) {
        if (direction === 'down') {
          nn.activeNode = $node.data('nid');
          nn.updatePageInfo($node);
        }
      },
      exited: function(direction) {
        // Trigger the scroll-out event.
        $node.trigger('nextnode.scrollout', direction);
      },
      offset: {
        top: nn.options.scrollOffset,
        bottom: nn.options.scrollOffset
      }
    });
  },
  // Initializes the sticky title functionality.
  initSticky: function() {
    var nn = this;
    // Set DOM elements.
    this.stickyElement = $('#nextnode-sticky', this.context);
    // Ensure we have the element available before continuing. This may happen
    // if the sticky title block has not been assigned to a theme region.
    if (!this.stickyElement.length) {
      this.options.stickyTitle = false;
      return;
    }
    this.stickyTitleElement = $('#nextnode-sticky-title', this.context);
    this.stickyMeterElement = $('#nextnode-sticky-meter', this.context);
    // Apply CSS properties.
    if (this.options.stickyZindex !== null) {
     this.stickyElement.css('z-index', this.options.stickyZindex);
    }
    if (this.options.stickyTop !== null) {
     this.stickyElement.css('top', this.options.stickyTop + 'px');
    }
    // Use Waypoint's Sticky shortcut to show/hide the title element.
    var Sticky = new Waypoint.Sticky({
      element: this.stickyElement[0],
      stuckClass: 'nextnodesticky--active',
      wrapper: '<div class="nextnodesticky__wrapper" />',
      offset: this.options.stickyOffset
    });
    // Reuse Waypoint's scroll event to update the progress meter.
    $(Sticky.waypoint.context.element).on('scroll.waypoints', function() {
      if (nn.activeNode === null) {
        // We haven't entered any scrollable content yet.
        return false;
      }
      var $this = $(this);
      // Find the node active in the viewport.
      $(nn.options.contentSelector, nn.context).each(function() {
        var $node = $(this);
        if ($node.data('nid') === nn.activeNode) {
          nn.resizeMeter($node, $this.scrollTop());
          return false;
        }
      });
    });
  },
  // Binds the click event for manual loading.
  loadOnClick: function() {
    var nn = this;
    $(this.options.nextSelector).on('click', function(event) {
      event.preventDefault();
      // Trigger the waypoint handler manually.
      nn.infinite.waypoint.callback();
      return false;
    // Ensure the link is visible.
    }).show();
  },
  // Updates the width of the sticky title percent meter.
  resizeMeter: function($item, windowTop) {
    var elementTop = $item.offset().top - this.options.scrollOffset;
    var itemHeight = $item.height();
    var elementBottom = elementTop + itemHeight;
    var percent = 0;
    if (windowTop >= elementTop && windowTop <= elementBottom) {
      percent = ((windowTop - elementTop) / itemHeight || 0) * 100;
    }
    else if (windowTop > elementBottom) {
      percent = 100;
    }
    this.stickyMeterElement.css('width', percent + '%');
  },
  // Changes page title and URL.
  updatePageInfo: function($node) {
    var title = $node.data('title');
    // Proceed only if the title has changed.
    if (this.titleElement.text() !== title) {
      if (this.options.updatePageInfo) {
        // Set history.
        history.pushState(null, title, $node.data('url'));
        // Set title.
        this.titleElement.html(title);
      }
      // Update sticky title.
      if (this.options.stickyTitle) {
        this.stickyTitleElement.html(title);
      }
    }
  },
  // Increases node stats counter.
  updateStats: function($node) {
    $.ajax({
      type: "POST",
      cache: false,
      url: this.options.statsURL,
      data: {"nid" : $node.data('nid')}
    });
  },
  // Tracks Google Analytics page views.
  trackPageView: function($node) {
    ga('set', 'page', $node.data('path'));
    ga('set', 'title', $node.data('title'));
    ga('send', 'pageview');
  }
};

Drupal.behaviors.nextnode = {
  attach: function(context, settings) {
    if (settings && typeof (settings.NextNode) !== 'undefined') {
      $(settings.NextNode.hookSelector, context).once('nextnode', function() {
        // We'll need the statistics URL to invoke the stats counter.
        if (typeof (settings.statistics) !== 'undefined') {
          settings.NextNode.statsURL = settings.statistics.url;
        }
        // Load NextNode.
        Drupal.nextNode.init(this, context, settings);
      });
    }
  }
};

}(jQuery));
