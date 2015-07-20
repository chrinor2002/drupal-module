;(function($){
  var item_loaded = false;
  $(document).ready(function(){

    $('.gc_tooltip').tooltip();

    $('tr[data-item-id]').each(function(){
      var parent_id = $(this).attr('data-parent-id'),
        item_id = $(this).attr('data-item-id'),
        parent_field = $('#gc_parent_' + item_id),
        import_as = $('#gc_import_as_' + item_id + ' input'),
        remove = true;

      if(parent_id > 0) {
        if($('tr[data-item-id="' + parent_id + '"]').length > 0) {
          remove = false;
        }
      }

      if(remove === true) {
        parent_field.find('.imported-item').remove();
      }
      set_value(parent_field);
    });

    $('td.gc_checkbox :checkbox').change(function() {
      var value = $(this).val(),
        type = $('#gc_import_as_' + value + ' input').val(),
        func = 'addClass';

      if($(this).is(':checked')) {
        if(typeof Drupal.settings.gathercontent.hierarchical_post_types[type] != 'undefined') {
          func = 'removeClass';
        }
      }

      hide_imported_parent(value, func);
    });

    $('input[name^="parent"]').change(function() {
      var func = 'addClass',
        item_id = $(this).closest('tr').attr('data-item-id');
      if($(this).val() != 0) {
        func = 'removeClass';
      }
      hide_imported_parent(item_id, func);
    });


    $('.repeat_config input').change(function(){
      var $t = $(this);
      if($t.is(':checked')){
        $('.gc_overlay,.gc_repeating_modal').show();
        setTimeout(function(){
          repeat_config($t);
        },500);
      }
    });

    $('.gc_field_map').find('ul.dropdown-menu a').click(function(){
      var $t = $(this),
          field = $t.closest('.gc_field_map'),
          tr = field.closest('tr'),
          item_id= tr.attr('data-item-id');
      if($('#gc_repeat_'+item_id).is(':checked')){
        var rows = tr.parent().find('tr[data-item-id]'),
            idx = rows.index(tr),
            field_id = field.attr('id').split('_')[4],
            val = $t.attr('data-value');
        rows.filter(':gt('+idx+')').each(function(){
          var item_id = $(this).attr('data-item-id');
          if(!$('#gc_repeat_'+item_id).is(':checked')){
            $('#gc_field_map_'+item_id+'_'+field_id+' li:not(.hidden-item) a[data-value="'+val+'"]').trigger('click');
          } else {
            return false;
          }
        });
      }
    });

    $('#gathercontent-item-import-form').submit(submit_item_import);

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

    $('.gc_settings_container .has_input').on('click', 'ul a', function(e){
      e.preventDefault();
      $(this).closest('.has_input').find('a:first span:first').html($(this).html()).end().siblings('input').val($(this).attr('data-value')).trigger('change');
    });

    $('.gc_import_as input').change(function(){
      var v = $(this).val(),
        c = $(this).closest('tr'),
        item_id = c.attr('data-item-id'),
        to = $('#gc_import_to_'+item_id),
        parent = $('#gc_parent_'+item_id);

      to.add(parent).find('li[data-post-type]').filter('[data-post-type!="'+v+'"]').hide().addClass('hidden-item').end().filter('[data-post-type="'+v+'"]').show().removeClass('hidden-item');

      set_value(to);

      if(typeof Drupal.settings.gathercontent.hierarchical_post_types[v] != 'undefined') {
        parent.show();
        set_value(parent);
        parent_func = 'show';
      }
      else {
        parent.hide();
        parent_func = 'hide';
      }

      hide_imported_parent(item_id, parent_func);

      set_map_to_fields(c,v,item_id);
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
        var tr = ui.item.closest('tr'),
          item_id = tr.attr('data-item-id');
        if($('#gc_repeat_'+item_id).is(':checked')){
          var rows = tr.parent().find('tr[data-item-id]'),
            idx = rows.index(tr),
            new_index = ui.item.index();
          rows.filter(':gt('+idx+')').each(function(){
            var $t = $(this),
                item_id = $t.attr('data-item-id'),
                field_id = ui.item.attr('id').split('_')[2],
                item = $('#field_'+item_id+'_'+field_id);
            if(!$('#gc_repeat_'+item_id).is(':checked')){
              if(item.length > 0){
                if(new_index > 0){
                  item.parent().find('> .gc_settings_field:eq('+(new_index > item.index() ? new_index : (new_index-1))+')').after(item);
                } else {
                  item.parent().prepend(item);
                }
              }
            } else {
              return false;
            }
          });
        }
      }
    });
    item_loaded = true;
  });

  function hide_imported_parent(item_id, func){

    var display = func == 'addClass' ? 'none' : 'list-item';

    $('#gc_itemlist tr[data-parent-id="' + item_id + '"]').each(function() {
      $(this).find('.imported-item')[func]('hidden-item').css('display', display);
      set_value($('#gc_parent_' + $(this).attr('data-item-id')));
    });
  };

  function set_map_to_fields(elem,v,item_id){
    var to = $('#gc_import_to_'+item_id),
        to_val = to.find('input').val(),
        m = $('#gc_fields_'+item_id+' div.gc_field_map');
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

    var is_parent = false;

    if(typeof elem.attr('id') != 'undefined') {
      if(elem.attr('id').indexOf('gc_parent_') === 0) {
        is_parent = true;
      }
    }

    if(elem.not(':visible') && !is_parent){
      elem.find('input:not(.live_filter):first').val('');
    }
    if(el.length == 0){
      el = elem.find('li:not(.hidden-item) a:first');
    }
    el.trigger('click');
  };

  function repeat_config($t){
    var c = $t.closest('tr'),
      item_id = c.attr('data-item-id'),
      table = $('#gc_itemlist'),
      rows = table.find('tbody tr[data-item-id]'),
      idx = rows.index(c),
      field_rows = c.find('.gc_settings_field'),
      fields = {},
      import_as = $('#gc_import_as_'+item_id+' input').val(),
      filter = $('#gc_filter_'+item_id+' input').val();
    rows = rows.filter(':gt('+idx+')');
    field_rows.each(function(){
      var $t = $(this).removeClass('not-moved'),
        id = $t.attr('id').split('_')[2];
      fields[field_rows.index($t)] = [$t.find('.gc_field_map input[name*="map_to"]').val(),id];
    });

    rows.each(function(){
      var $t = $(this),
        item_id = $t.attr('data-item-id'),
        c = $('#gc_fields_'+item_id);
      if(!$('#gc_repeat_'+item_id).is(':checked')){
        c.find('> .gc_settings_field').removeClass('moved').addClass('not-moved');
        $('#gc_import_as_'+item_id+' a[data-value="'+import_as+'"]').trigger('click');
        $('#gc_filter_'+item_id+' a[data-value="'+filter+'"]').trigger('click');
        for(var i in fields){
          if(fields.hasOwnProperty(i)){
            $('#gc_field_map_'+item_id+'_'+fields[i][1]+' li:not(.hidden-item) a[data-value="'+fields[i][0]+'"]').trigger('click');
            var field = $('#field_'+item_id+'_'+fields[i][1]).removeClass('not-moved').addClass('moved');
            if(i > 0){
              c.find('> .gc_settings_field:eq('+(i-1)+')').after(field);
            } else {
              c.prepend(field);
            }
          }
        };
      } else {
        return false;
      }
    });
    $('.gc_overlay,.gc_repeating_modal').hide();
  }

  var save = {
    "total": 0,
    "cur_counter": 0,
    "els": null,
    "waiting": null,
    "progressbar": null,
    "title": null,
    "cur_retry": 0
  };
  function submit_item_import(e){
    e.preventDefault();
    save.els = $('#gc_itemlist td.gc_checkbox :checkbox:checked');
    save.total = save.els.length;
    save.cur_counter = 0;
    save.waiting = $('.gc_importing_modal img');
    save.progressbar = $('#current_item .bar');
    save.title = $('#gc_item_title');
    if(save.total > 0){
      $('.gc_overlay,.gc_importing_modal').show();
      save_item();
    }
    return false;
  };

  function save_item(){
    $.ajax({
      url: Drupal.settings.gathercontent.ajaxurl,
      data: get_item_data(save.els.filter(':eq('+save.cur_counter+')')),
      dataType: 'json',
      type: 'POST',
      timeout: 120000,
      beforeSend: function(){
        save.waiting.show();
      },
      error: function(){
        save.waiting.hide();
        if(save.cur_retry == 0){
          save.cur_retry++;
          save_item();
        } else {
          alert(Drupal.settings.gathercontent.error_message);
          $('.gc_overlay,.gc_importing_modal').hide();
        }
      },
      success: function(data){
        save.waiting.hide();
        if(typeof data.error != 'undefined'){
          save.cur_retry++;
          alert(data.error);
          $('.gc_overlay,.gc_importing_modal').hide();
        }
        if(typeof data.success != 'undefined'){
          save.cur_retry--;

          if(typeof data.new_item_html != 'undefined') {

            $('#gc_itemlist tr[data-parent-id="'+data.item_id+'"]').each(function(){
              var el = $('#gc_parent_'+$(this).attr('data-item-id')),
                input = el.find('input');

              if($(this).find('a[data-value="'+data.new_item_id+'"]').length == 0)
              {
                el.find('ul').append(data.new_item_html);
              }
              if(input.val() == '_imported_item_') {
                input.val(data.new_item_id);
                set_value(el);
              }
            });
          }

          save.cur_retry = 0;
          save.cur_counter++;
          save.progressbar.css('width',data.item_percent+'%');
          if(save.cur_counter == save.total){
            setTimeout(function(){
              window.location.href = Drupal.settings.gathercontent[data.redirect_url];
            },1000);
          } else {
            setTimeout(save_item,1000);
          }
        }
      }
    });
  };

  function get_item_data($t){
    var tr = $t.closest('tr'),
      title = tr.find('td.item-name span').text(),
      item_id = $t.val(),
      settings = $('#gc_fields_'+item_id),
      data = {
        "cur_retry": save.cur_retry,
        "cur_counter": save.cur_counter,
        "total": save.total
      },
      title_text = title;
    if(title_text.length > 30){
      title_text = title_text.substring(0,27)+'...';
    }
    save.title.attr('title',title).text(title_text);
    if(settings.length > 0){
      data.gc = {
        "item_id": item_id,
        "post_type": $('#gc_import_as_'+item_id+' input').val(),
        "overwrite": $('#gc_import_to_'+item_id+' input').val(),
        "filter": $('#gc_filter_'+item_id+' input').val(),
        "parent_id": $('#gc_parent_'+item_id+' input').val(),
        "fields": []
      };
      settings.find('> .gc_settings_field').each(function(){
        var $t = $(this),
          field = {
            "field_tab": $t.find('input[name^="field_tab["]').val(),
            "field_name": $t.find('input[name^="field_name["]').val(),
            "map_to": $t.find('input[name^="map_to["]').val()
          };
        data.gc.fields.push(field);
      });
    }
    return data;
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
