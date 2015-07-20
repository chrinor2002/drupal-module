;(function($){
  var itemlist, itemlist_c;
  $(document).ready(function() {
    $('.gc_url_prefix, .gc_url_suffix').click(function(){
      $('#edit-gathercontent-api-url').focus();
    });
    $('.gc_toggle_all').change(function(){
      var checked = $(this).is(':checked');
      $('.gc_toggle_all').prop('checked',checked);
      $('#gc_itemlist td > div > :checkbox').prop('checked',false).filter(':visible').prop('checked',checked).trigger('change');
    });
    $('.gc_search_items .gc_right .dropdown-menu a').click(function(e){
      e.preventDefault();
      if($(this).attr('data-custom-state-name') == 'All'){
        $('table tbody tr:not(:visible)').show();
      } else {
        var selector = '[data-item-state="'+$(this).attr('data-custom-state-id')+'"]';
        $('table tbody tr .item-status').filter(':not('+selector+')').closest('tr').hide().end().end().filter(selector).closest('tr').show();
      }
      $(this).closest('.btn-group').find('> a span:first').text($(this).attr('data-custom-state-name'));
    });
    $('#gc_live_filter').keyup(function(){
      var v = $.trim($(this).val()), items = $('#gc_itemlist tbody tr');
      if(!v || v == ''){
        items.show();
      } else {
        v = v.toLowerCase();
        console.log(items.find('.item-name span').length);
        items.find('.item-name span').each(function(){
          var e = $(this), t = e.text().toLowerCase(),
            show = (t.indexOf(v) > -1), func = (show?'show':'hide');
          e.closest('tr')[func]();
        });
      }
    }).change(function(){$(this).trigger('keyup')});

    itemlist = $('#gathercontent-item-form #gc_itemlist tr td');
    itemlist_c = $('#gathercontent-item-form');
    itemlist.click(function(e){
      if(!$(e.target).is(':checkbox')){
        var el = $(this).closest('tr').find(':checkbox');
        el.prop('checked',(el.is(':checked')?false:true)).trigger('change');
      }
    });

    itemlist.find(':checkbox').change(function(){
      var el = $(this).closest('tr'),
        checked = $(this).is(':checked');
      el[(checked?'addClass':'removeClass')]('selected');
    }).trigger('change');
  });
})(window.$gc_jQuery);
