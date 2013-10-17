;(function($){
  var page_loaded = false;
  $(document).ready(function(){
    $('.repeat_config input').change(function(){
      var $t = $(this);
      if($t.is(':checked')){
        var c = $t.closest('tr'),
          page_id = c.attr('data-page-id'),
          table = $('#gc_pagelist'),
          rows = table.find('tbody tr[data-page-id]'),
          idx = rows.index(c),
          field_rows = c.find('.gc_settings_field'),
          fields = {},
          import_as = $('#gc_import_as_'+page_id+' input').val(),
          filter = $('#gc_filter_'+page_id+' input').val();
        rows = rows.filter(':gt('+idx+')');
        field_rows.each(function(){
          var $t = $(this).removeClass('not-moved'),
            id = $t.attr('id').split('_')[2];
          fields[field_rows.index($t)] = [$t.find('.gc_field_map input[name*="map_to"]').val(),id];
        });

        rows.each(function(){
          var $t = $(this),
            page_id = $t.attr('data-page-id'),
            c = $('#gc_fields_'+page_id);
          c.find('> .gc_settings_field').addClass('not-moved');
          $('#gc_import_as_'+page_id+' a[data-value="'+import_as+'"]').trigger('click');
          $('#gc_filter_'+page_id+' a[data-value="'+filter+'"]').trigger('click');
          for(var i in fields){
            if(fields.hasOwnProperty(i)){
              $('#gc_field_map_'+page_id+'_'+fields[i][1]+' li:not(.hidden-item) a[data-value="'+fields[i][0]+'"]').trigger('click');
              var field = $('#field_'+page_id+'_'+fields[i][1]).removeClass('not-moved');
              if(i > 0){
                c.find('> .gc_settings_field:eq('+(i-1)+')').after(field);
              } else {
                c.prepend(field);
              }
            }
          };
          var new_ids = [];
          $t.find('.gc_settings_field').each(function(){
            new_ids.push($(this).attr('data-field-index'));
          });
          $t.find('.gc_field-order input').val(new_ids.join(','));
        });
      }
    });

    $('#gathercontent-pages-import-form').submit(function(){
      $('.gc_overlay,.gc_modal').show();
    });
    $('.page-settings a').click(function(e){
      e.preventDefault();
      var el = $(this).closest('tr').next().find('> td > div');
      if(el.is(':visible')){
        el.slideUp('fast').fadeOut('fast',function(){
          el.parent().hide();
        });
        $(this).find('.caret').addClass('caret-up');
      } else {
        el.parent().show().end().slideDown('fast').fadeIn('fast');
        $(this).find('.caret').removeClass('caret-up');
      }
    });

    $('.gc_field_map input.live_filter').click(function(e){
      e.preventDefault();
      e.stopImmediatePropagation();
    }).keyup(function(e){
      var v = $(this).val(),
        lis = $(this).parent().siblings('li:not(.hidden-item):not(.divider)');
      if(!v || v == ''){
        lis.show();
      } else {
        lis.hide().filter('[data-search]:icontains_searchable('+$(this).val()+')').show();
      }
    }).focus(function(){
      $(this).trigger('keyup');
    });

    $('.gc_settings_container .has_input ul a').click(function(e){
      e.preventDefault();
      $(this).closest('.has_input').find('a:first span:first').html($(this).html()).end().siblings('input').val($(this).attr('data-value')).trigger('change');
    });

    $('.gc_import_as input').change(function(){
      var v = $(this).val(),
        c = $(this).closest('tr'),
        page_id = c.attr('data-page-id'),
        to = $('#gc_import_to_'+page_id);
      to.find('li[data-post-type]').filter('[data-post-type!="'+v+'"]').hide().addClass('hidden-item').end().filter('[data-post-type="'+v+'"]').show().removeClass('hidden-item');
      set_value(to);
      set_map_to_fields(c,v,page_id);
    }).each(function(){
      set_value($(this).parent());
    });

    $('.gc_filter').each(function(){
      set_value($(this));
    });

    $('.gc_settings_container').sortable({
      items: 'div.gc_settings_field',
      handle: '.gc_move_field',
      update: function(e, ui) {
        var new_ids = [],
            el = ui.item.closest('.gc_settings_container');
          el.find('.gc_settings_field').each(function(){
          new_ids.push($(this).attr('data-field-index'));
        });
        el.find('.gc_field-order input').val(new_ids.join(','));
      }
    });
    page_loaded = true;
  });

  function set_map_to_fields(elem,v,page_id){
    var to = $('#gc_import_to_'+page_id),
        to_val = to.find('input').val(),
        m = $('#gc_fields_'+page_id+' div.gc_field_map');
    m.each(function(){
      $(this).find('li').filter(':not([data-post-type="'+v+'"])')
        .hide().addClass('hidden-item')
      .end().filter('[data-post-type="all"],[data-post-type="'+v+'"]')
        .show().removeClass('hidden-item');
      set_value($(this),true);
      var length = $(this).find('li:not(.live_filter):not(:is_hidden)').length;
      $(this).find('li.live_filter')[(length > 13? 'show' : 'hide')]();
    });
  };

  function set_value(elem){
    var v = elem.find('input:not(.live_filter):first').val(),
      el = elem.find('li:not(.hidden-item) a[data-value="'+v+'"]:first');
    if(elem.not(':visible')){
      elem.find('input:not(.live_filter):first').val('');
    }
    if(el.length == 0){
      el = elem.find('li:not(.hidden-item) a:first');
    }
    el.trigger('click');
  };
  window.$gc_jQuery.expr[":"].icontains_searchable = window.$gc_jQuery.expr.createPseudo(function(arg) {
      return function( elem ) {
          return window.$gc_jQuery(elem).attr('data-search').toUpperCase().indexOf(arg.toUpperCase()) >= 0;
      };
  });
  window.$gc_jQuery.expr[":"].is_hidden = window.$gc_jQuery.expr.createPseudo(function() {
      return function( elem ) {
          return window.$gc_jQuery(elem).css('display') == 'none';
      };
  });
})(window.$gc_jQuery);
