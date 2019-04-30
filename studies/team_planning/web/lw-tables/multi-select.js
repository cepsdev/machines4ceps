

let lw_tables_multi_select = (function () {
    let class_object = undefined;
    ctrls = [];
    id_counter = 0;
    return class_object = {
       new_ctrl : function () {
         let ctrl_obj = undefined;
         ctrls.push(ctrl_obj = { 
             id : id_counter++,
             columns:0,
             container: undefined,
             min_width      : "600px",             
             elems : {
                 cur_search_string : undefined,
                 count          : function () {return 0;},
                 get            : function (i) {return undefined;},
                 visible        : function (i) {
                                        return undefined;
                                    },
                 selected       : function (i) {return false;},
                 all_selected   : function () {return true;},
                 title          : function (i) {return {en:"",de:""};},
                 toggle         : function (i) {}
             },
             toggle_entry : function(i){
                if (i < 0){
                    let sel_status = $('#lw-tables-multi-select-sel-ctrl-'+ctrl_obj.id+'-'+ -1).prop("checked");
                    let v = [];
                    for(let i = 0; i < ctrl_obj.elems.count();++i)
                    {
                        if (ctrl_obj.elems.visible(i) && ctrl_obj.elems.selected(i) != sel_status){
                            v.push(i);
                            $('#lw-tables-multi-select-sel-'+ctrl_obj.id+'-'+ i).prop("checked",sel_status);
                        }
                    }
                    ctrl_obj.elems.select(v,sel_status);
                    return;
                }
                ctrl_obj.elems.toggle(i);
                let all_sel = ctrl_obj.elems.all_selected();
                $('#lw-tables-multi-select-sel-ctrl-'+ctrl_obj.id+'-'+ -1).prop("checked",all_sel);
             },
             search_text_changed: function (){
                
                ctrl_obj.elems.cur_search_string = $("#lw-tables-search-multi-select-search-string-"+ctrl_obj.id).val().trim();
                if(ctrl_obj.elems.cur_search_string.length == 0) {
                    ctrl_obj.elems.cur_search_string  = ctrl_obj.elems.cur_regexp = undefined;
                } else ctrl_obj.elems.cur_regexp = RegExp(ctrl_obj.elems.cur_search_string ,"i");
                if (ctrl_obj.elems.text_changed != undefined) ctrl_obj.elems.text_changed();
                let html = class_object.gen_html(ctrl_obj,ctrl_obj);
                $(ctrl_obj.selection_elements_target).html(html[1]);                
             }
         });
         return ctrl_obj;
       },
       get_ctrl :function (id) {
           for(let i = 0; i < ctrls.length; ++i)
             if (id == ctrls[i].id) return ctrls[i];
           return undefined;
       },
       gen_html : function (ctrl_obj,options) {
                    all_selected = ctrl_obj.elems.all_selected() ? "checked" : "";
                    let result = [];
                    let r = "";
                    let placeholder = 'placeholder="Search" placeholder-lang-en="Search" placeholder-lang-de="Suche" ';
                    if (options != undefined && options.search_text != undefined && options.search_text.trim().length > 0)
                     placeholder = 'value="'+options.search_text+'"';
                    r = '<div class="input-group" style="width:100%;padding:5px">'+
                        '<input '+
                                'type="text" '+
                                'id="lw-tables-search-multi-select-search-string-'+ctrl_obj.id+'" '+
                                'onkeyup="lw_tables_multi_select.get_ctrl('+ctrl_obj.id+').search_text_changed();" '+
                                'class="form-control lang-aware-text-input" '+ 
                                placeholder+
                         '> '+
                                '<span class="input-group-addon"><i class="glyphicon glyphicon-search"></i></span>'+
                       '</div>';
                    result.push(r);r="";
                    let no_select_all = false;
                    if (options != undefined && options.no_select_all != undefined ) no_select_all = options.no_select_all;

                    if (!no_select_all) r += '<div class="row" style="padding-left:10px;padding-top:5px;">'+
                      '<div class="col-md-12" style="min-width:'+ctrl_obj.min_width+';">' +      
                       '<div class="row">'+               
                        '<div class="col-sm-6">'+

                         '<div class="pretty p-default" style="margin-right:4px;">'+
                         '<input type="checkbox"'+
                         '       onchange="lw_tables_multi_select.get_ctrl('+ctrl_obj.id+').toggle_entry(-1);" '+
                         '       id="lw-tables-multi-select-sel-ctrl-'+ctrl_obj.id+'-'+ -1 +'" ' + all_selected +'/>'+
                         '<div class="state">'+
                        '<label><i>'+build_lang_aware_text_elem({"de":"Alle ausw&auml;hlen",
                                            "en":"Select all"
                                            },language_context.current)+'</i></label>'+
                        '</div>'+
                       '</div>'+

                    '</div></div></div></div>';

                    let style_overflow_scroll = "scroll";
                    if (options != undefined && options.scroll != undefined && !options.scroll) style_overflow_scroll = "none";

                    r += '<div style="width:100%;height:'+ctrl_obj.main_part_height+';overflow:'+style_overflow_scroll+'">';

                    for(let i = 0; i < ctrl_obj.elems.count();){
                        cols = [];
                        for(; i < ctrl_obj.elems.count();++i)
                         if (!ctrl_obj.elems.visible(i)) continue;
                         else {cols.push(i); if (cols.length >= ctrl_obj.columns){++i; break;}}

                        r += '<div class="row" style="padding-left:10px;padding-top:5px;">'+
                                '<div class="col-md-12" style="min-width'+ctrl_obj.min_width+';">' +      
                                    '<div class="row">';
                        for(let k = 0; k < cols.length;++k){
                                     let selected = ctrl_obj.elems.selected(cols[k]) ? "checked" : "";
                                     r += '<div class="col-sm-'+Math.floor(12/ctrl_obj.columns)+'">'+
                                        '<div class="pretty p-default" style="margin-right:4px;">'+
                                            '<input type="checkbox" onchange="lw_tables_multi_select.get_ctrl('+ctrl_obj.id+').toggle_entry('+cols[k]+');"'+
                                            '       id="lw-tables-multi-select-sel-'+ctrl_obj.id+'-'+cols[k]+'"'+selected+'/>'+
                                            '<div class="state">'+
                                             '<label>'+
                                                build_lang_aware_text_elem(ctrl_obj.elems.title(cols[k]),language_context.current)+
                                             '</label>'+
                                            '</div>'+
                                        '</div>'+
                                     '</div>';
                        }
                        r +=        '</div>'+
                                '</div>'+
                             '</div>';
                    }
                    r+= '</div>';
                    result.push(r);
                    return result;
                },
     create: function (container_search_input,container_sel_elements,elements,columns,options){
         let ctrl = class_object.new_ctrl();
         ctrl.search_input_target = container_search_input;
         ctrl.selection_elements_target = container_sel_elements;
         if (elements != undefined) ctrl.elems = elements;
         if (columns != undefined) ctrl.columns = columns;
         ctrl.main_part_height = "100px";
         for(let v in options)
          ctrl[v] = options[v];
         let html = class_object.gen_html(ctrl,options);
         $(container_search_input).html(html[0]);
         $(container_sel_elements).html(html[1]);
         return ctrl;
     }
 };
})();
