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
        if ($('#edit-nextnode-override', context).is(':checked')) {
          var profile = $('#edit-nextnode-profile option:selected').text();
          return Drupal.t('Overriding profile: !profile.', {'!profile':profile});
        }
        else {
          return Drupal.t('Using the default profile');
        }
      });
    }
  };

})(jQuery);
