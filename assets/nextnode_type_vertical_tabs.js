/**
 * @file
 * Code to provide summary information inside vertical tabs.
 */

(function ($) {

  /**
   * Provide summary information for vertical tabs.
   */
  Drupal.behaviors.nextnode_settings = {
    attach: function (context) {
      // Provide summary during content type configuration.
      $('fieldset#edit-nextnode', context).drupalSetSummary(function (context) {
        var vals = [];
        if ($('#edit-nextnode-enable', context).is(':checked')) {
          vals.push(Drupal.t('NextNode is enabled.'));
          var profile = $('#edit-nextnode-profile option:selected').text();
          vals.push(Drupal.t('Default profile: !profile.', {'!profile':profile}));
          if ($('#edit-nextnode-override', context).is(':checked')) {
            vals.push(Drupal.t('Profile overriding is enabled.'));
          }
        }
        else {
          vals.push(Drupal.t('NextNode is disabled'));
        }
        return vals.join('<br/>');
      });
    }
  };

})(jQuery);
