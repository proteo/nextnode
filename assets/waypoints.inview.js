/**
 * @file
 * Custom Waypoints Inview offset configurations.
 *
 * @link https://github.com/imakewebthings/waypoints/issues/450
 */

Waypoint.Inview.prototype.createWaypoints = function() {
  var configs = {
    vertical: [{
      down: 'enter',
      up: 'exited',
      offset: function() {
        var _offset = this.options.offset && this.options.offset.bottom || 0;
        return this.options.context.innerHeight - _offset;
      }.bind(this)
    }, {
      down: 'entered',
      up: 'exit',
      offset: function() {
        var _offset = this.options.offset && this.options.offset.bottom || 0;
        return this.options.context.innerHeight - jQuery(this.element).outerHeight() - _offset;
      }.bind(this)
    }, {
      down: 'exit',
      up: 'entered',
      offset: function() {
        var _offset = this.options.offset && this.options.offset.top || 0;
        return _offset;
      }.bind(this)
    }, {
      down: 'exited',
      up: 'enter',
      offset: function() {
        var _offset = this.options.offset && this.options.offset.top || 0;
        return _offset - jQuery(this.element).outerHeight();
      }.bind(this)
    }],
  };

  for (var i = 0, end = configs[this.axis].length; i < end; i++) {
    var config = configs[this.axis][i]
    this.createWaypoint(config)
  }
};
