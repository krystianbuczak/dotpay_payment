(function ($) {
  /*
  * Allow check all checkboxes in fieldset
  * only for testing purposes
   */
  Drupal.behaviors.selectAll = {
    attach: function (context, settings) {
      $("legend").click(function() {
      	// TODO test with table grouping and check all
      	// if so we will don't need this file
        $(this).next().children(".form-checkboxes").find('input[type="checkbox"]').attr('checked','checked');
        $(this).attr('data-checked', 'all');
      });
    }
  };
})(jQuery);