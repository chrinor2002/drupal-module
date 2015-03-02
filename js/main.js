;(function($){
  var pagelist, pagelist_c;
  $(document).ready(function() {
    $('.gc_url_prefix, .gc_url_suffix').click(function(){
      $('#edit-gathercontent-api-url').focus();
    });
    $('.gc_toggle_all').change(function(){
      var checked = $(this).is(':checked');
      $('.gc_toggle_all').prop('checked',checked);
      $('#gc_pagelist td > div > :checkbox').prop('checked',false).filter(':visible').prop('checked',checked).trigger('change');
    });
    $('.gc_search_pages .gc_right #filter-state-dropdown .dropdown-menu a').click(function(e){
      e.preventDefault();
      if($(this).attr('data-custom-state-name') == 'All'){
        $('table tbody tr:not(:visible)').show();
      } else {
        var selector = '[data-page-state="'+$(this).attr('data-custom-state-id')+'"]';
        $('table tbody tr .page-status').filter(':not('+selector+')').closest('tr').hide().end().end().filter(selector).closest('tr').show();
      }
      $(this).closest('.btn-group').find('> a span:first').text($(this).attr('data-custom-state-name'));
    });
    $('.gc_search_pages .gc_right #filter-page-level .dropdown-menu a').click(function(e){
      e.preventDefault();
      if($(this).attr('data-page-level') == '-1'){
        $('table tbody tr:not(:visible)').show();
      } else {
        var max_level = $(this).attr('data-page-level');
        var selector = function(index){
          return $(this).data('page-level') <= max_level;
        };
        $('table tbody tr .page-level').filter(function(index){
          return !selector.call(this);
        }).closest('tr').hide().end().end().filter(selector).closest('tr').show();
      }
      $(this).closest('.btn-group').find('> a span:first').text($(this).attr('data-page-level'));
    });
    $('#gc_live_filter').keyup(function(){
      var v = $.trim($(this).val()), items = $('#gc_pagelist tbody tr');
      if(!v || v == ''){
        items.show();
      } else {
        v = v.toLowerCase();
        console.log(items.find('.page-name span').length);
        items.find('.page-name span').each(function(){
          var e = $(this), t = e.text().toLowerCase(),
            show = (t.indexOf(v) > -1), func = (show?'show':'hide');
          e.closest('tr')[func]();
        });
      }
    }).change(function(){$(this).trigger('keyup')});

    pagelist = $('#gathercontent-pages-form #gc_pagelist tr td');
    pagelist_c = $('#gathercontent-pages-form');
    pagelist.click(function(e){
      if(!$(e.target).is(':checkbox')){
        var el = $(this).closest('tr').find(':checkbox');
        el.prop('checked',(el.is(':checked')?false:true)).trigger('change');
      }
    });

    pagelist.find(':checkbox').change(function(){
      var el = $(this).closest('tr'),
        checked = $(this).is(':checked');
      el[(checked?'addClass':'removeClass')]('selected');
    }).trigger('change');
  });
})(window.$gc_jQuery);
