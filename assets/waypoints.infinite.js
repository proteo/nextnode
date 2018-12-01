/**
 * @file
 * Override Waypoints' Infinite setup handler.
 *
 * This is basically a copy of the original function definition, excepting that
 * it removes the use of $.parseHTML() and appends the whole response to the
 * container element, not just individual items. We do this in order to preserve
 * other elements (like <script>) possibly coming along in the response.
 */

(function() {
  'use strict'

  var $ = window.jQuery
  var Waypoint = window.Waypoint

  Waypoint.Infinite.prototype.setupHandler = function() {
    this.options.handler = $.proxy(function() {
      this.options.onBeforePageLoad();
      this.destroy();
      this.$container.addClass(this.options.loadingClass);

      $.get($(this.options.more).attr('href'), $.proxy(function(data) {
        var $data = $(data);

        // Extract the NextNode selector before inserting the response.
        var $newMore = $data.filter(this.options.more).detach();

        this.$container.append($data);
        this.$container.removeClass(this.options.loadingClass);

        if ($newMore.length) {
          this.$more.replaceWith($newMore);
          this.$more = $newMore;
          this.waypoint = new Waypoint(this.options);
        }
        else {
          this.$more.remove();
        }

        // Now pass the collection of individual items to the callback function,
        // as the original function does.
        var $items = $data.find(this.options.items);
        if (!$items.length) {
          $items = $data.filter(this.options.items)
        }
        this.options.onAfterPageLoad($items)
      }, this))
    }, this)
  }
}());
