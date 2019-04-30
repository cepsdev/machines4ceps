

let lw_tables = (function () {
    let cur_resize_col_idx = -1;
    let cur_tbl_id = -1;
    let cur_resize_col = undefined;
    let initial_page_x = undefined;
    let initial_width = undefined;
    let initial_width_left = undefined;
    let tbl_infos = [];
    let self = undefined;
    let cur_w = undefined;

    function move_elem_to(v,from,to){
        if (from == to) return;
        let step = from > to ? 1 : -1;
        let t_v = v[to];
        let f_v = v[from];         
        for(let i = to; i != from; i += step){
            let tt_v = v[i + step];
            v[i + step] = t_v;
            t_v= tt_v;
        }
        v[to] = f_v;
    }

    function move_elem_to_with_functors(from,to,ge,se){
        if (from == to) return;
        let step = from > to ? 1 : -1;
        let t_v = ge(to);
        let f_v = ge(from);         
        for(let i = to; i != from; i += step){
            let tt_v = ge(i + step);
            se(i + step, t_v);
            t_v= tt_v;
        }
        se(to,f_v);
    }

    return self = {
    
       SORT_ASCENDING       : 1,
       SORT_DESCENDING      : 2,
       SORT_NONE            : 0,
       filter_dlg_container : "filter_dlg",
       
       create_data_source_from_matrix : function(m,c){
            let self = undefined;
            return self = {
                m_ : m,
                c_ : c,
                rv_ : [],
                row_count : function () { if (self.m_==undefined) return 0; return self.m_.length;},
                col_count : function () { if (self.m_==undefined || self.m_.length == 0) return 0; return self.m_[0].length;}, 
                elem : function (i,j){
                    return self.m_[i][j]; 
                },
                col: function (i) {if (self.c_ == undefined) return ""; return self.c_[i];},
                move_col_to : function(from,to){
                    if (from == to) return;
                    if (self.m_ == undefined) return;
                    if (self.c_ == undefined) return;
                    move_elem_to(self.c_,from,to);
                    for(let i = 0; i < self.row_count();++i){
                        move_elem_to(self.m_[i],from,to);
                    }
                },
                col_type : function (col) {return "string";},
                col_type_given_col_id : function (col_id) {return "string";},
                row_visible : function (r,b) {
                    if (self.rv_ == undefined || self.rv_.length != self.row_count()){
                        self.rv_ = [];
                        for(let i = 0; i < self.row_count(); ++i) self.rv_.push(true);
                    }
                    let t = self.rv_[r];
                    if (b != undefined) self.rv_[r] = b;
                    return t;
                },
                get_set_of_visible_entries_in_col:function (col,filter){
                    let r = {};
                    for(let i = 0; i < self.row_count();++i){
                        if (!self.row_visible(i)) continue;
                        if (filter && !filter(self.elem(i,col))) continue;
                        if (r[self.elem(i,col)] == undefined) r[self.elem(i,col)] = [i];
                        else r[self.elem(i,col)].push(i);
                    }
                    let v = [];
                    for(let k in r) v.push(k);                    
                    return [v,r];
                },
                get_col_by_id : function(col_id){
                    for(let col = 0;col < self.col_count();++col) if (self.col(col).id == col_id) return col;
                }
             }
       },
       create_default_html_gen : function(tbl_id) {
           let self = undefined;
           return self = {

            tbl_hdr: function(start_col,end_col){
                let tbl_info = lw_tables.get_table(tbl_id);
                if (end_col == undefined) end_col = tbl_info.data_source.col_count();
                let total_w = 0;
                for(let i = start_col; i < end_col;++i) total_w += tbl_info.col_w[i];
                let hdr =           
                '<table '+
                    'class = "lw-tables-header" '+
                    'style="table-layout: fixed;overflow:hidden;'+
                    'height:'+tbl_info.tbl_header_height+'px;'+
                    'width:0px;'+
                    'border:none;">'+
                        '<tr style="height:'+tbl_info.tbl_header_height+'px;'+
                                    'border:none;">';
                for(let i = start_col; i < end_col; ++i){
                    let vis = true;
                    if (tbl_info.data_source.col(i).visible != undefined)vis = tbl_info.data_source.col(i).visible;
                    let w = tbl_info.col_w[i]-tbl_info.hdr_dummy_column_width;
                    let w_thumb  = tbl_info.hdr_dummy_column_width;
                    //if (!vis) {w = 0;w_thumb=0;}

                    hdr+='<td   id="lw-tables-header-'+tbl_id+'-'+i+'" '+
                            '   style="height:'+tbl_info.tbl_header_height+'px;'+
                                    'width:'+w+'px;'+
                                    'overflow:hidden;'+
                                    (vis?'':'display:none;')+
                                    '" '+
                                'class ="lw-tables-th lw-tables-no-select lw-tables-cursor-default" '+
                                ' '+
                            '>'+ 
                            tbl_info.data_source.col(i).html(i,tbl_info) +
                            '</td>'+
                        '<td '+
                            ' id="lw-tables-width-resize-thumb-'+tbl_id+'-'+i+'"' +
                            ' class="lw-tables-width-resize-th"'+
                            ' '+
                            ' onmousedown="lw_tables.resize_thumb_mousedown(event,'+tbl_id+','+i+');" '+
                            ' style="xborder:none;'+
                                    'height:'+tbl_info.tbl_header_height+'px;'+
                                    (vis?'':'display:none;')+
                                    'width:'+w_thumb+'px;"'+
                        '>'+ 
                            ''+
                        '</td>';
                }
                hdr += "</tr></table>";
                return hdr;
           },
           tbl_body_td: function(tbl_info,tbl_id,i,j){
               let s = 
               '<td '+
                    'id="lw-tables-td-'+tbl_id+'-'+i+'-'+j+'" '+
                    'class="lw-tables-no-overflow lw-tables-td" '+
                    'style="'+
                            'width:'+(tbl_info.col_w[j])+'px;'+
                            //'background-color:red;'+
                            //'overflow:hidden;'+
                            'overflow:hidden;'+
                          '"'+
                '>';
                s+='<div style="'+
                                ''+
                               '"> ';
                s+=tbl_info.data_source.elem(i,j);
                s+='</div>';
                s+="</td>";
                return s;
           },
           tbl_body: function(start_row,end_row,start_col,end_col){
            let tbl_info = lw_tables.get_table(tbl_id);
            if (start_col == undefined) start_col = 0;
            if (end_col == undefined) end_col = tbl_info.data_source.col_count();
            if (start_row == undefined) start_row = 0;
            if (end_row == undefined) end_row = tbl_info.data_source.row_count();
            let total_width = 0;
            for(let j = start_col; j < end_col;++j) total_width += tbl_info.col_w[j];
            
            let s = 
            '<table '+
            ' class = "lw-tables-data" '+
            ' style = "'+
                      'table-layout: fixed;'+
                      'width:'+0+'px;'+
                      'overflow:hidden;'+
                      '"'+
            '>';

            for(let i = start_row; i < end_row; ++i){
                s+='<tr class="lw-tables-data-tr" style="xborder:none;" >';
                for(let j = start_col; j < end_col;++j){
                    s+=self.tbl_body_td(tbl_info,tbl_id,i,j);
                }
                s+="</tr>";
            }

            s += "</table>";

            return s;
       }
       }},
       get_next_unassigned_tbl_id : function () {return tbl_infos.length;},
       toggle_pin : function(tbl_id,id) {
        let tbl_info = lw_tables.get_table(tbl_id);
        for(let i = 0; i < tbl_info.data_source.col_count();++i)
         if (tbl_info.data_source.col(i).id == id){
             if (i < tbl_info.fixed_columns)
              tbl_info.unpin_column(i);
            else 
              tbl_info.pin_column(i,"left");
             break;
         }           
       },
       toggle_sort : function(ev,tbl_idx,pivot_col){
           ev.preventDefault();
           let tbl_info = lw_tables.get_table(tbl_idx);
           tbl_info.toggle_sort(pivot_col);
       },
       create_table : function(d_source,html_generator){
         let instance = undefined;
         tbl_infos.push(instance = {
             col_w : [],
             row_heights : undefined,
             col_min_width : 50,
             number_of_rows : function () {return 0;},
             col_cache: [],
             hdr_dummy_column_width : 16,
             fixed_columns : 0,
             tbl_id : tbl_infos.length,
             display_left_width : 0,
             display_right_width : 0,
             tbl_header_height : 0,
             data_source : d_source,
             row_sort : { pivot_col:-1, 
                          ordering:lw_tables.SORT_NONE,
                          order:undefined},
             row_filters : undefined,
             filter_stack : [],
             horiz_scroll_bar_height : 18,
             horiz_vert_bar_width : 8,
             tbl_left_pos : 0,
             ignore_resize_event: function () {return true;},
             fit_layout: function () {},

            prevent_text_selection:function(){
                let ti = instance;
                let tbl_id = ti.tbl_id;
                $('#lw-tables-data-outer-right-'+tbl_id).addClass("lw-tables-no-select");
                $('#lw-tables-hdr-outer-right-'+tbl_id).addClass("lw-tables-no-select");
                $('#lw-tables-data-outer-left-'+tbl_id).addClass("lw-tables-no-select");
                $('#lw-tables-hdr-outer-left-'+tbl_id).addClass("lw-tables-no-select");
            },

            allow_text_selection:function(){
                let ti = instance;
                let tbl_id = ti.tbl_id;
                $('#lw-tables-data-outer-right-'+tbl_id).removeClass("lw-tables-no-select");
                $('#lw-tables-hdr-outer-right-'+tbl_id).removeClass("lw-tables-no-select");
                $('#lw-tables-data-outer-left-'+tbl_id).removeClass("lw-tables-no-select");
                $('#lw-tables-hdr-outer-left-'+tbl_id).removeClass("lw-tables-no-select");
            },

            //FILTERING
            filter_active : function(col){
                let ti = instance;
                if (ti.row_filters == undefined) return false;
                if (ti.data_source.col_type(col) != "date"){
                    let filt_ext = ti.get_row_filter_ext(col);
                    if (filt_ext == undefined) return false;
                    let v = filt_ext.selected_values_orig;
                    if ( v == undefined) v = filt_ext.selected_values;
                    if ( v == undefined) return false;
                    for(let i = 0; i < v.length; ++i) if (!v[i]) return true;
                    return false;
                }
                return false;               
            },

            number_of_active_filters : function () {
                let r = 0;let ti = instance;
                for(let i = 0; i < ti.data_source.col_count();++i)
                 if (ti.filter_active(i)) ++r;
                return r;
            },

            fresh_filter : function(){
                let ti = instance;
                if (ti.row_filters == undefined) return undefined;
                let rf = ti.row_filters;
                for(let i = 0;i < rf.length;++i) 
                 if (ti.data_source.col_type(i) != "date")
                  if (rf[i].ext.selected_values != undefined && rf[i].ext.selected_values_orig == undefined) return {filter:rf[i],idx:i};
                return undefined;
            },

            setup_filter :function (col_id){
                let ti = instance;
                let col = ti.data_source.get_col_by_id(col_id);
                let filt_ext = ti.get_row_filter_ext(col);
                if (ti.data_source.col_type(col) != "date"){
                    if (filt_ext.possible_values == undefined){
                        let possible_values_ext = ti.data_source.get_set_of_visible_entries_in_col(col);
                        filt_ext.possible_values = possible_values_ext[0];
                        filt_ext.possible_values_ext = possible_values_ext[1];
                        filt_ext.selected_values = [];
                        for(let i = 0; i < filt_ext.possible_values.length; ++i) filt_ext.selected_values.push(true);
                    }
                }
                return filt_ext;
            },

            remove_filter_from_stack : function(col_id){
                let ti = instance;
                for(let i = 0; i < ti.filter_stack.length;++i ) if (ti.filter_stack[i]==col_id){
                    ti.filter_stack.splice(i,1);
                    break;
                }
            },

            modify_filter : function (col_id,historize) {
                let ti = instance;
                let col = ti.data_source.get_col_by_id(col_id);
                let filter = ti.row_filters[col];
                if (!filter.in_filter_stack){
                    ti.filter_stack.push(col_id);
                    filter.in_filter_stack = true;
                }
                if (historize) filter.ext.selected_values_orig = filter.ext.selected_values;
                ti.apply_row_filters();
            },
            get_row_filter_ext : function(col){
                let ti = instance;
                if (ti.row_filters == undefined) {ti.row_filters = []; for(let i = 0; i < ti.data_source.col_count();++i)ti.row_filters.push( {hide_rows:[],ext:{}}); }
                return ti.row_filters[col].ext;                
            },
            add_row_filter : function(col,rfilt,ext){
                let ti = instance;
                if (ti.row_filters == undefined) {ti.row_filters = []; for(let i = 0; i < ti.data_source.col_count();++i)ti.row_filters.push( {hide_rows:[],ext:{}}); }
                ti.row_filters[col].hide_rows = rfilt;
                ti.row_filters[col].ext = ext;
                ti.apply_row_filters();
            },
            apply_row_filters: function() {
                let ti = instance;
                console.log(ti.filter_stack);

                let r = {};
                for(let i = 0;i < ti.filter_stack.length;++i){
                    let col_id = ti.filter_stack[i];
                    let col = ti.data_source.get_col_by_id(col_id);
                    let rf = ti.row_filters[col];
                    let filt_ext = rf.ext;
                    let v = [];
                    if (ti.data_source.col_type(col) != "date"){
                        for(let j = 0;j < filt_ext.selected_values.length;++j)
                         if (!filt_ext.selected_values[j]) v = v.concat(filt_ext.possible_values_ext[filt_ext.possible_values[j]]);
                    }
                    rf.hide_rows = v;
                    for(let j = 0; j < rf.hide_rows.length;++j) r[rf.hide_rows[j]] = true;
                }
                let v = [];
                for(let k in r) v.push(parseInt(k));
                let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+ti.tbl_id).childNodes[1].childNodes[0];
                let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+ti.tbl_id).childNodes[1].childNodes[0];
                let j = 0;
                for(let i = 0; i  < ti.data_source.row_count();++i){
                    if (v.length <= j || v[j] != i){
                        $(tr_elems_left_data.childNodes[i]).css("display","table-row");
                        $(tr_elems_right_data.childNodes[i]).css("display","table-row");
                        ti.data_source.row_visible(i,true);
                    } else {
                        ++j;
                        $(tr_elems_left_data.childNodes[i]).css("display","none");
                        $(tr_elems_right_data.childNodes[i]).css("display","none");
                        ti.data_source.row_visible(i,false);
                    }
                }
            },
            hide_row : function(r){
                let ti = instance;
                let tbl_id = ti.tbl_id;                
                let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+ti.tbl_id).childNodes[1].childNodes[0];
                let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+ti.tbl_id).childNodes[1].childNodes[0];
                if (r.length == undefined){
                    if (tr_elems_left_data.childNodes.length > r) $(tr_elems_left_data.childNodes[r]).css("display","none");
                    if (tr_elems_right_data.childNodes.length > r) $(tr_elems_right_data.childNodes[r]).css("display","none");
                } else {
                    for(let i = 0; i < r.length; ++i){
                        $(tr_elems_left_data.childNodes[r[i]]).css("display","none");
                        $(tr_elems_right_data.childNodes[r[i]]).css("display","none");
                    }
                }
            },                                              

            //
            //
            //SORTING

            toggle_sort : function(pivot_col /*column id, not positional*/){
                let ti = instance;
                if (ti.row_sort.pivot_col > -1 && pivot_col != ti.row_sort.pivot_col){
                    let btn = $('#lw-tables-th-default-btn-sort-'+ti.tbl_id+'-'+ti.row_sort.pivot_col);
                    let icon = $('#lw-tables-th-default-glyph-sort-'+ti.tbl_id+'-'+ti.row_sort.pivot_col);
                    $(btn).removeClass("lw-tables-sort").removeClass("lw-tables-sort-ascending").removeClass("lw-tables-sort-descending").addClass("lw-tables-sort");
                    $(icon).removeClass("glyphicon-sort").removeClass("glyphicon-sort-by-attributes").removeClass("glyphicon-sort-by-attributes-alt").addClass("glyphicon-sort");
                    ti.row_sort.ordering = lw_tables.SORT_NONE;
                    ti.order = undefined;
                }
                
                let btn = $('#lw-tables-th-default-btn-sort-'+ti.tbl_id+'-'+pivot_col);
                let icon = $('#lw-tables-th-default-glyph-sort-'+ti.tbl_id+'-'+pivot_col);
                //console.log('#lw-tables-th-default-glyph-sort-'+ti.tbl_id+'-'+pivot_col);

                $(btn).removeClass("lw-tables-sort").removeClass("lw-tables-sort-ascending").removeClass("lw-tables-sort-descending");
                $(icon).removeClass("glyphicon-sort").removeClass("glyphicon-sort-by-attributes").removeClass("glyphicon-sort-by-attributes-alt");
                if (ti.row_sort.ordering == lw_tables.SORT_NONE){
                    $(btn).addClass("lw-tables-sort-ascending");
                    $(icon).addClass("glyphicon-sort-by-attributes");
                    ti.row_sort.ordering = lw_tables.SORT_ASCENDING;
                    ti.row_sort.pivot_col = pivot_col;
                } else if (ti.row_sort.ordering == lw_tables.SORT_ASCENDING){
                    $(btn).addClass("lw-tables-sort-descending");
                    $(icon).addClass("glyphicon-sort-by-attributes-alt");
                    ti.row_sort.ordering = lw_tables.SORT_DESCENDING;
                    ti.row_sort.pivot_col = pivot_col;
                } else {
                    $(btn).addClass("lw-tables-sort");
                    $(icon).addClass("glyphicon-sort");
                    ti.row_sort.ordering = lw_tables.SORT_NONE;
                    ti.row_sort.pivot_col = pivot_col;
                }
                if (ti.row_sort.order == undefined){
                    ti.row_sort.order = [];
                    for(let i = 0; i < ti.data_source.row_count();++i)
                     ti.row_sort.order.push(i);
                }                
                let col = undefined;
                let col_idx = -1;
                for(let i = 0; i < ti.data_source.col_count();++i )
                 if (ti.data_source.col(i).id == ti.row_sort.pivot_col){col = ti.data_source.col(i);col_idx=i; break;}
                if (col == undefined) return;
                let tt = [];for(let i = 0; i < ti.data_source.row_count();++i)tt.push(i);


                if (ti.row_sort.ordering != lw_tables.SORT_NONE ){
                    let compare_f = col.compare_fct(ti.row_sort.ordering);
                    tt.sort(
                        function (a,b){
                            return compare_f(ti.data_source.elem(a,col_idx),ti.data_source.elem(b,col_idx));
                        }
                );}
                let tl = [];
                let tr = [];
                for(let i = 0; i < tt.length;++i){ 
                    for(let j = 0; j < ti.row_sort.order.length;++j)
                     if (ti.row_sort.order[j] == tt[i] ) {tl.push(j);tr.push(j); break;} 
                }
                
                let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+ti.tbl_id).childNodes[1].childNodes[0];
                let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+ti.tbl_id).childNodes[1].childNodes[0];
               
                for(let i = 0; i < tr.length;++i) {tr[i] = tr_elems_right_data.childNodes[tr[i]];tl[i] = tr_elems_left_data.childNodes[tl[i]];}
                while(tr_elems_right_data.childNodes.length) tr_elems_right_data.removeChild(tr_elems_right_data.firstChild);
                while(tr_elems_left_data.childNodes.length) tr_elems_left_data.removeChild(tr_elems_left_data.firstChild);

                for(let i = 0; i < tr.length;++i) tr_elems_right_data.appendChild(tr[i]);
                for(let i = 0; i < tl.length;++i) tr_elems_left_data.appendChild(tl[i]);

                ti.row_sort.order = [];
                for(let i = 0; i < tt.length;++i) ti.row_sort.order.push(tt[i]);
                
            },
             
             get_column_by_idx : function(idx){
                 for (let i = 0; i < instance.data_source.col_count();++i)
                  if (instance.data_source.col(i).id==idx) return i;
                return -1;
             },
             column : function (i) {
                 let self = undefined;
                 return self = {
                     ref : instance.data_source.col(i),
                     visible : function(b,draw_header) {
                         let ti = instance;
                         if (self.ref.visible == undefined) self.ref.visible = true;
                         if (draw_header == undefined) draw_header = true;
                         if (b != undefined){
                             let t = self.ref.visible;
                             self.ref.visible = b;
                             if (i >= ti.fixed_columns){
                              if(draw_header)
                              {
                                  let tbl_hdr_right = instance.html_gen.tbl_hdr(instance.fixed_columns);
                                $('#lw-tables-hdr-inner-right-'+instance.tbl_id).html(tbl_hdr_right);
                              }
                              let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+instance.tbl_id).childNodes[1].childNodes[0];
                              let n = tr_elems_right_data.childNodes.length;
                              for(let j = 0; j < n;++j){
                                 let childs_j = tr_elems_right_data.childNodes[j];
                                 childs_j.childNodes[i-instance.fixed_columns].style.display= (b?"":"none");
                              }
                             } else {
                                if(draw_header){
                                    let tbl_hdr_left = instance.html_gen.tbl_hdr(0,instance.fixed_columns);
                                    $('#lw-tables-hdr-inner-left-'+instance.tbl_id).html(tbl_hdr_left);
                                }
                                let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+instance.tbl_id).childNodes[1].childNodes[0];
                                let n = tr_elems_left_data.childNodes.length;
                                for(let j = 0; j < n;++j){
                                    let childs_j = tr_elems_left_data.childNodes[j];
                                    childs_j.childNodes[i].style.display= (b?"":"none");
                                }                              
                             }
                             return t;
                         }
                         return self.ref.visible;
                     }

                 };
             },
             columns : function (cols) {
               if (cols != undefined && cols.length){
                   return {
                       visible: function(what){
                        for(let i = 0; i < cols.length-1;++i){
                            instance.column(cols[i]).visible(what,false);
                        }
                        instance.column(cols[cols.length-1]).visible(what,true);
                       }
                    }
                   
               } else   return {
                   visible:function(){
                       let v = [];
                       for(let i = 0; i < instance.data_source.col_count();++i){
                            if (instance.data_source.col(i).visible == undefined)
                             instance.data_source.col(i).visible = true;
                            v.push(instance.data_source.col(i).visible);
                       }
                       return v;
                   }

               }
             },
             html_gen : html_generator != undefined ? html_generator : self.create_default_html_gen(tbl_infos.length), 
             width_left : function () {
                 let r = 0;
                 for(let i = 0; i < instance.fixed_columns;++i)
                  r += instance.col_w[i];
                  return r;
             },
             width_right : function () {
                let r = 0;
                for(let i = instance.fixed_columns;i < instance.col_w.length;++i)
                  r += instance.col_w[i];
                return r;
             },
             get_dom_rep_of_elem:function (i,j){
                 if (j < instance.fixed_columns){
                     let t = document.getElementById('lw-tables-data-inner-left-'+instance.tbl_id).childNodes[1].childNodes[0];
                     return t.childNodes[i].childNodes[j];
                 } else {
                     let t = document.getElementById('lw-tables-data-inner-right-'+instance.tbl_id).childNodes[1].childNodes[0];
                     return t.childNodes[i].childNodes[j-instance.fixed_columns];
                 }              
             },
             pin_column : function (which,direction){
                 if (direction != "left") return;
                 if (which < instance.fixed_columns) return;
                 if (instance.data_source == undefined) return;
                 let d_left = instance.col_w[which];
                 for(let i = 0; i < instance.fixed_columns; ++i)
                  d_left += instance.col_w[i];
                 instance.data_source.move_col_to(which,instance.fixed_columns);
                 move_elem_to(instance.col_w,which,instance.fixed_columns);
                 //if (instance.row_sort.pivot_col == which) instance.row_sort.pivot_col = instance.fixed_columns;        
                 ++instance.fixed_columns;
                 let tbl_hdr_left = instance.html_gen.tbl_hdr(0,instance.fixed_columns);
                 let tbl_hdr_right = instance.html_gen.tbl_hdr(instance.fixed_columns);
                 
                 $('#lw-tables-hdr-inner-left-'+instance.tbl_id).html(tbl_hdr_left);
                 $('#lw-tables-hdr-inner-right-'+instance.tbl_id).html(tbl_hdr_right);

                 let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+instance.tbl_id).childNodes[1].childNodes[0];
                 let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+instance.tbl_id).childNodes[1].childNodes[0];

                 let n = tr_elems_right_data.childNodes.length;
                 if (instance.row_heights == undefined){
                    instance.row_heights = [];
                    for(let i = 0; i < n;++i){
                        let childs_right_i = tr_elems_right_data.childNodes[i];
                        let row_height = $(childs_right_i).css("height");
                        instance.row_heights.push(row_height);
                     }
                 }

                 for(let i = 0; i < n;++i){
                    let childs_i = tr_elems_right_data.childNodes[i];
                    childs_i.removeChild(childs_i.childNodes[which-instance.fixed_columns+1]);
                 }

                 n = tr_elems_left_data.childNodes.length;
                 for(let i = 0; i < n;++i){
                     let s = instance.html_gen.tbl_body_td(instance,instance.tbl_id,i,instance.fixed_columns-1);
                     tr_elems_left_data.childNodes[i].insertAdjacentHTML('beforeend',s);
                 }
                 
                 for(let i = 0; i < instance.row_heights.length;++i){
                    let childs_right_i = tr_elems_right_data.childNodes[i];
                    let childs_left_i = tr_elems_left_data.childNodes[i];
                    let row_height = instance.row_heights[i];
                    $(childs_left_i).css("height",row_height);
                    $(childs_right_i).css("height",row_height);
                 }


                 instance.adjust_widths_outer_blocks(d_left);
             },
             unpin_column : function(which){
                let ti = instance;
                if (ti.fixed_columns <= which) return;
                let dt = ti.data_source;
                let nl = -ti.col_w[which];
                for(let i = 0; i < ti.fixed_columns; ++i)
                 nl += ti.col_w[i];
                instance.data_source.move_col_to(which,instance.fixed_columns-1);
                move_elem_to(instance.col_w,which,instance.fixed_columns-1);
                //if (instance.row_sort.pivot_col == which) instance.row_sort.pivot_col = instance.fixed_columns-1;                
                --instance.fixed_columns;
                let tbl_hdr_left = instance.html_gen.tbl_hdr(0,instance.fixed_columns);
                let tbl_hdr_right = instance.html_gen.tbl_hdr(instance.fixed_columns);
                
                $('#lw-tables-hdr-inner-left-'+instance.tbl_id).html(tbl_hdr_left);
                $('#lw-tables-hdr-inner-right-'+instance.tbl_id).html(tbl_hdr_right);

                let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+instance.tbl_id).childNodes[1].childNodes[0];
                let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+instance.tbl_id).childNodes[1].childNodes[0];


                let n = tr_elems_left_data.childNodes.length;
                for(let i = 0; i < n;++i){
                   let childs_i = tr_elems_left_data.childNodes[i];
                   childs_i.removeChild(childs_i.childNodes[which]);
                }
                n = tr_elems_right_data.childNodes.length;
                for(let i = 0; i < n;++i){
                    let s = instance.html_gen.tbl_body_td(instance,instance.tbl_id,i,instance.fixed_columns);
                    tr_elems_right_data.childNodes[i].insertAdjacentHTML('afterbegin',s);
                }
                ti.adjust_widths_outer_blocks(nl);
                let col_pin_btn = $('#lw-tables-th-default-btn-pin-'+ti.tbl_id+'-'+ti.data_source.col(which).id);
                $(col_pin_btn).removeClass("lw-tables-unpin").removeClass("lw-tables-pin").addClass("lw-tables-pin");
             }, 
             drag_column : function(from,to){
                let ti = instance;
                let dt = ti.data_source;

                function update_title_and_data(){
                    dt.move_col_to(from,to);
                    move_elem_to(ti.col_w,from,to);                
                    let tbl_hdr_left = ti.html_gen.tbl_hdr(0,ti.fixed_columns);
                    let tbl_hdr_right = ti.html_gen.tbl_hdr(ti.fixed_columns);
                
                    $('#lw-tables-hdr-inner-left-'+ti.tbl_id).html(tbl_hdr_left);
                    $('#lw-tables-hdr-inner-right-'+ti.tbl_id).html(tbl_hdr_right);
                }

                function move_data_cols(data,ofs){
                    let n = data.childNodes.length;
                    for(let i = 0; i < n;++i){
                        let childs_i = data.childNodes[i];
                        move_elem_to_with_functors(from+ofs,
                                                   to+ofs,
                                     function(j){return childs_i.childNodes[j].innerHTML;},
                                     function(j,t){return childs_i.childNodes[j].innerHTML = t;} );
                        let a = from; let b = to;
                        if (from > to) {a = to;b = from;}
                        for(let j = a; j != b+1;++j){
                            childs_i.childNodes[j+ofs].style.width = ti.col_w[j]+"px";
                        }
                    }
                    
                }
                if (from >= ti.fixed_columns && to >= ti.fixed_columns){
                    update_title_and_data();
                    move_data_cols( document.getElementById('lw-tables-data-inner-right-'+instance.tbl_id).childNodes[1].childNodes[0],
                                    -ti.fixed_columns);
                } else if (from < ti.fixed_columns && to < ti.fixed_columns){
                    update_title_and_data();
                    move_data_cols( document.getElementById('lw-tables-data-inner-left-'+instance.tbl_id).childNodes[1].childNodes[0],
                                    0);
                } else if (from < ti.fixed_columns && to >= ti.fixed_columns){
                    ti.unpin_column(from);
                    from = ti.fixed_columns;
                    update_title_and_data();
                    move_data_cols( document.getElementById('lw-tables-data-inner-right-'+instance.tbl_id).childNodes[1].childNodes[0],
                                    -ti.fixed_columns);
                } else if (to < ti.fixed_columns && from >= ti.fixed_columns){
                    ti.pin_column(from,"left");
                    from = ti.fixed_columns-1;
                    update_title_and_data();
                    move_data_cols( document.getElementById('lw-tables-data-inner-left-'+instance.tbl_id).childNodes[1].childNodes[0],
                                    0);
                }
             },
             adjust_widths_outer_blocks: function (new_left,new_right){
                 //console.log("adjust_widths_outer_blocks");
                 //console.log("new_left:",new_left);
                 //console.log("new_right:",new_right);                 
                 let d = new_left - instance.display_left_width;
                 //console.log("d:",d);
                 if (new_right == undefined)
                  new_right = instance.display_right_width - d;
                 //console.log("new_right:",new_right);

                 let hdr_left =  $('#lw-tables-hdr-outer-left-'+instance.tbl_id);
                 let hdr_right =  $('#lw-tables-hdr-outer-right-'+instance.tbl_id);
                 let data_right = $('#lw-tables-data-outer-right-'+instance.tbl_id);
                 let data_left = $('#lw-tables-data-outer-left-'+instance.tbl_id);

                 $(hdr_left).css("width",new_left);
                 $(hdr_right).css("left", instance.tbl_left_pos+new_left);
                 $(hdr_right).css("width",new_right);

                 $(data_right).css("left", instance.tbl_left_pos+new_left);                     
                 $(data_right).css("width",new_right);
                 $(data_left).css("width", new_left); 

                 instance.display_left_width = new_left;
                 instance.display_right_width = new_right;
             },
            
             sort_active : function(column) {                
                let ti = instance;
                if (column == instance.row_sort.pivot_col) return instance.row_sort.ordering;
                return lw_tables.SORT_NONE;
   }
         });
         return tbl_infos.length-1;
     },
     get_table : function(idx){
         return tbl_infos[idx];
     },
     set_col_width: function(tbl_id,col_idx,w){
        let tbl_info = self.get_table(tbl_id);
         for(let i = 0; i < tbl_info.col_cache.length;++i){
            $(tbl_info.col_cache[i]).css("width",w);
         }
     },
     
     resize_thumb_mousedown : function (ev,tbl_id,col_idx){
        
        //$("body").css("cursor","ew-resize");
        $("body").css("pointer-events","none");

        let cur_resize_col_idx = col_idx;
        let ti = self.get_table(tbl_id);
        let initial_width_left = ti.width_left();
        let cur_resize_col = $("#lw-tables-header-"+tbl_id+"-"+col_idx);
        ti.prevent_text_selection();
        let ruler = document.getElementById("lw-tables-internal-resize-ruler");
        if(ruler == undefined){
            ruler = 
            '<div '+
             'id="lw-tables-internal-resize-ruler"'+
             'style="'+
             'z-index:1000000;'+
             'cursor : col-resize;'+
             'position:absolute;'+
             'height:50px;'+
             'top:0px;'+
             'left:'+ev.pageX+'px;'+
             //'pointer-events:auto;'+
             //'width:1px;'+
             //'background-color:black;'+
             'border-left: dotted 2px;'+
             //'transition: color 1.5s;'+
             //'background-color:blue;'+
             '"'+
             '>'+
             '<span '+
             'style="'+
             'background-color:white;'+
             'opacity:0.5;'+
             'font-weight:bold;'+
             'border-radius:4px;'+
             '"'+
             'id="ruler-info">'+
             '</span>'+
            '</div>';
           $("body").append(ruler);
           ruler = document.getElementById("lw-tables-internal-resize-ruler");
        }
        let ruler_info = document.getElementById("ruler-info");
        let h1 = document.getElementById('lw-tables-hdr-outer-right-'+tbl_id).style.height;
        let h2 = document.getElementById('lw-tables-data-outer-right-'+tbl_id).style.height;
        ruler.style.left = ev.pageX+"px";
        ruler.style.height = (parseFloat(h1) + parseFloat(h2) - ti.horiz_scroll_bar_height)+"px";
        ruler.style.top = document.getElementById('lw-tables-hdr-outer-right-'+tbl_id).style.top;
        ruler.style.display = "block";
        let update_pos = false;
        let ev_pageX = ev.pageX;
        /*let update_proc = setInterval(function(){ 
            if (update_pos){
                update_pos = false;
                ruler.style.left = ev_pageX+"px";
            }
         }, 100);*/
        selected_col_elem = document.getElementById("lw-tables-header-"+tbl_id+"-"+col_idx);
        selected_col_elem_bounding_rect = selected_col_elem.getBoundingClientRect();
        //console.log("lw-tables-header-"+tbl_id+"-"+col_idx,selected_col_elem_bounding_rect);

        cur_resize_left = selected_col_elem_bounding_rect.left;//document.getElementById("lw-tables-header-"+tbl_id+"-"+col_idx).offsetLeft;

        
        //if (ti.fixed_columns <= col_idx) cur_resize_left+=cur_left_width();

        //console.log("col_idx:",col_idx);
        //console.log("cur_resize_left:",cur_resize_left);
        let min_left = cur_resize_left+ti.col_min_width;
        let max_left = document.getElementById('lw-tables-hdr-outer-left-'+tbl_id).offsetWidth + 
                       document.getElementById('lw-tables-hdr-outer-right-'+tbl_id).offsetWidth - ti.horiz_vert_bar_width;
                      /*document.getElementById('lw-tables-hdr-outer-right-'+tbl_id).offsetLeft + 
                       document.getElementById('lw-tables-hdr-outer-right-'+tbl_id).offsetWidth - ti.horiz_vert_bar_width;*/
        //console.log("cur_resize_left",cur_resize_left,"min_left",min_left,"max_left",max_left);

        ti.col_cache = [];
        for(let i = 0; i < ti.number_of_rows();++i){
          ti.col_cache.push(ti.get_dom_rep_of_elem(i,col_idx));
        }

        $(ruler_info).html(ti.col_w[cur_resize_col_idx]+"px");
        
        function cur_left_width(){
            let r = 0;
            for(let i = 0; i < ti.fixed_columns;++i)
             r += ti.col_w[i];
            //for(let i = 0; i < ti.fixed_columns;++i)
            //    if(i != cur_resize_col_idx)  r += ti.col_w[i];
            //if (ti.fixed_columns > cur_resize_col_idx ) r += cur_w;
            return r;
        }
        
        $(window).mousemove(function(ev){
            //$(ruler).css("left",ev.pageX);
            
            update_pos = true;
            ev_pageX = ev.pageX;
            
            let paint_red = false;
            if (min_left > ev_pageX) {paint_red = true;ev_pageX = min_left;}
            if (ev_pageX > max_left) {paint_red = true;ev_pageX = max_left;}
            ruler.style.left = ev_pageX+"px"; 
            $(ruler_info).html(Math.floor(ev_pageX - cur_resize_left)+"px"); 
            if (paint_red) $(ruler_info).css("color","red");
            else $(ruler_info).css("color","");
        });
        $(window).mouseup(function(ev){
            let w = Math.floor(ev_pageX - cur_resize_left+1);
            if (ti.col_min_width < w){
             console.log("w:",w);
             $(cur_resize_col).css("width",w-ti.hdr_dummy_column_width);
             self.set_col_width(tbl_id,cur_resize_col_idx,w);                
            }
            ti.col_w[cur_resize_col_idx] = w;
            /*if (cur_left_width() != initial_width_left) */ ti.adjust_widths_outer_blocks(cur_left_width());



            ruler.style.display = "none";
            $("body").css("pointer-events","auto");
            $(window).unbind("mousemove");
            $(window).unbind("mouseup");
            ti.allow_text_selection();            
        });
     },

     resize_thumb_mousedown_2 : function (ev,tbl_id,col_idx){
        cur_resize_col_idx = col_idx;
        cur_tbl_id = tbl_id;
        cur_resize_col = $("#lw-tables-header-"+tbl_id+"-"+col_idx);
        initial_page_x = Math.floor(ev.pageX);
        initial_width = self.get_table(tbl_id).col_w[col_idx];
        initial_width_left = self.get_table(cur_tbl_id).width_left();
        let tbl_info = self.get_table(cur_tbl_id);
        let ti = tbl_info;
        tbl_info.col_cache = [];

        //let tbl_rows_height = [];
        let rh_max_left_without_changing_col = [];
        let rh_max_right_without_changing_col = [];
        let resize_row_cache = [];
        let resize_col_cache = [];

        for(let i = 0; i < ti.data_source.row_count();++i){rh_max_left_without_changing_col.push("0px");rh_max_right_without_changing_col.push("0px");}

        let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+ti.tbl_id).childNodes[1].childNodes[0];
        let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+ti.tbl_id).childNodes[1].childNodes[0];

        for(let i = 0; i < ti.data_source.row_count();++i){
            let tr_i = tr_elems_left_data.childNodes[i];
            //tbl_rows_height[i] = [];
            for(let j = 0; j < ti.fixed_columns; ++j){                
            // tbl_rows_height[i].push($(tr_i.childNodes[j].childNodes[0]).css("height"));
             let h = $(tr_i.childNodes[j].childNodes[0]).css("height");
             if (j == cur_resize_col_idx){
                resize_row_cache.push(tr_i);
                resize_col_cache.push(tr_i.childNodes[j].childNodes[0]);
                continue;
             }
             if (parseFloat(rh_max_left_without_changing_col[i]) < parseFloat(/*tbl_rows_height[i][j]*/h))
              rh_max_left_without_changing_col[i] =/* tbl_rows_height[i][j]*/h;
            }
        }

        for(let i = 0; i < ti.data_source.row_count();++i){
            //if (tbl_rows_height[i] == undefined) tbl_rows_height[i] = [];
            let tr_i = tr_elems_right_data.childNodes[i];
            for(let j = ti.fixed_columns; j < ti.data_source.col_count(); ++j){
             //tbl_rows_height[i].push($(tr_i.childNodes[j-ti.fixed_columns].childNodes[0]).css("height"));
             let h = $(tr_i.childNodes[j-ti.fixed_columns].childNodes[0]).css("height");
             if (j == cur_resize_col_idx){
                resize_row_cache.push(tr_i);
                resize_col_cache.push(tr_i.childNodes[j-ti.fixed_columns].childNodes[0]);
                continue;
             }
             if (parseFloat(rh_max_right_without_changing_col[i]) < parseFloat(/*tbl_rows_height[i][j]*/h))
              rh_max_right_without_changing_col[i] = /*tbl_rows_height[i][j]*/h;
            }
        }

        //console.log(rh_max_left_without_changing_col);
        //console.log(rh_max_right_without_changing_col);
        //console.log(cur_resize_col_idx);


        for(let i = 0; i < tbl_info.number_of_rows();++i){
            tbl_info.col_cache.push( tbl_info.get_dom_rep_of_elem(i,col_idx)/*$('#lw-tables-td-'+tbl_id+'-'+i+'-'+col_idx)*/);
        }
        tbl_info.prevent_text_selection();

        function cur_left_width(){
            let r = 0;
            for(let i = 0; i < self.get_table(cur_tbl_id).fixed_columns;++i)
                if(i != cur_resize_col_idx)  r += self.get_table(cur_tbl_id).col_w[i];
            if (self.get_table(cur_tbl_id).fixed_columns > cur_resize_col_idx ) r += cur_w;
            return r;
        }
        
        $(window).mousemove(function(ev){
            let ti = self.get_table(cur_tbl_id);
            let new_x = Math.floor(ev.pageX);
            let w = new_x - initial_page_x + initial_width;
            if (ti.col_min_width < w){
             cur_w = w;
             $(cur_resize_col).css("width",w-ti.hdr_dummy_column_width);
             self.set_col_width(cur_tbl_id,cur_resize_col_idx,cur_w);                
            }
            //console.log(cur_left_width());
            if (true) {                    
                
                if (ti.fixed_columns > 0){
                    let tr_elems_left_data = document.getElementById('lw-tables-data-inner-left-'+ti.tbl_id).childNodes[1].childNodes[0];
                    let tr_elems_right_data = document.getElementById('lw-tables-data-inner-right-'+ti.tbl_id).childNodes[1].childNodes[0];
                    ti.row_heights = [];
                    let n = resize_col_cache.length;
                    //console.log(n);
                    for (let i = 0; i < n;++i){
                        let t = $(resize_col_cache[i]).css("height");
                        console.log(t);
                        if (parseFloat(t) < parseFloat(rh_max_left_without_changing_col[i])) t = rh_max_left_without_changing_col[i];
                        if (parseFloat(t) < parseFloat(rh_max_right_without_changing_col[i])) t = rh_max_right_without_changing_col[i];
                        t = (parseFloat(t)+4)+"px";
                        ti.row_heights.push(t);
                        //console.log(t);
                    }
                    for (let i = 0; i < n;++i){
                        let childs_right_i = tr_elems_right_data.childNodes[i];
                        let childs_left_i = tr_elems_left_data.childNodes[i];

                        $(childs_right_i).css("height",ti.row_heights[i]);
                        $(childs_left_i).css("height",ti.row_heights[i]);
                    }
                }
            }
            if (cur_left_width() != initial_width_left) ti.adjust_widths_outer_blocks(cur_left_width());

        });

        $(window).mouseup(function(ev){
            $(window).unbind("mousemove");
            $(window).unbind("mouseup");
            let ti = self.get_table(cur_tbl_id);
            ti.allow_text_selection();

            let new_x = Math.floor(ev.pageX);
            let w = new_x - initial_page_x + initial_width;
            if (ti.col_min_width < w){
             cur_w = w;
             $(cur_resize_col).css("width",cur_w-ti.hdr_dummy_column_width);
             self.set_col_width(cur_tbl_id,cur_resize_col_idx,cur_w);     
             ti.col_w[cur_resize_col_idx] = cur_w;           
            }                 
        });
     },
     start_drag: function(ev,col_idx,tbl_id){
         ev.dataTransfer.setData("text",  JSON.stringify( {tbl_id:tbl_id , col_idx:col_idx}) );
     },
     allow_drop:function(ev,col_idx_to_drop_upon,tbl_id){
         if (ev.dataTransfer.getData("text") != undefined){
             let ti = self.get_table(tbl_id);
             let col = col_idx_to_drop_upon;//ti.data_source.get_col_by_id(col_idx_to_drop_upon);
             //console.log($('#lw-tables-header-'+tbl_id+'-'+col));
             let col_elem = $('#lw-tables-header-'+tbl_id+'-'+col);
             if ($(col_elem).hasClass("lw-tables-th")){
               $('#lw-tables-header-'+tbl_id+'-'+col).removeClass("lw-tables-th")
                                                   .removeClass("lw-tables-th-drag-over-another-th")
                                                   .addClass("lw-tables-th-drag-over-another-th");
               $('#lw-tables-width-resize-thumb-'+tbl_id+'-'+col).removeClass("lw-tables-width-resize-th").addClass("lw-tables-th-drag-over-another-th-resize-thumb");

             }
             //console.log(ev.dataTransfer.getData("text"));
             //let data = JSON.parse(ev.dataTransfer.getData("text"));
             //if (data.tbl_id == undefined || data.col_idx == undefined) return;
             ev.preventDefault();
             return true;
             //lw-tables-th-drag-over
         }
         return false;
     },
     drag_leave: function(ev,col_idx_to_drop_upon,tbl_id){
        if (ev.dataTransfer.getData("text") != undefined){
            let ti = self.get_table(tbl_id);
            let col = col_idx_to_drop_upon;//ti.data_source.get_col_by_id(col_idx_to_drop_upon);
            $('#lw-tables-header-'+tbl_id+'-'+col).removeClass("lw-tables-th")
                                                  .removeClass("lw-tables-th-drag-over-another-th")
                                                  .addClass("lw-tables-th");
            $('#lw-tables-width-resize-thumb-'+tbl_id+'-'+col).addClass("lw-tables-width-resize-th").removeClass("lw-tables-th-drag-over-another-th-resize-thumb");
            ev.preventDefault(); 
        }
     },
     drop: function(ev,col_idx_to_drop_upon){
        
        let data = JSON.parse(ev.dataTransfer.getData("text"));
        if (data.tbl_id == undefined || data.col_idx == undefined) return;
        ev.preventDefault();
        let tbl_info = self.get_table(data.tbl_id);
        tbl_info.drag_column(data.col_idx,col_idx_to_drop_upon);
    },
    close_filter_dlg:function(ev,tbl_id,col_id){
        let ti = self.get_table(tbl_id);
        $("#"+self.filter_dlg_container).css("display","none");
    },
    show_filter_dlg : function(ev,tbl_id,col_id){
        let ti = self.get_table(tbl_id);
        $("#"+self.filter_dlg_container).fadeIn(580);
        $("#"+self.filter_dlg_container).css("position","relative");
        $("#"+self.filter_dlg_container).css("display","block");
        $("#"+self.filter_dlg_container).offset({top:ev.clientY,left:ev.clientX});
        $("#"+self.filter_dlg_container).css("width","450px");
        //$("#"+self.filter_dlg_container).css("height","400px");
        //$("#"+self.filter_dlg_container).css("overflow","scroll");
        

        //let col = 0;
        //for(;col < ti.data_source.col_count();++col) if (ti.data_source.col(col).id == col_id) break;

        if ("date" != ti.data_source.col_type_given_col_id(col_id)){

                let filt_ext = ti.setup_filter(col_id);
                let historize = filt_ext.selected_values_orig == undefined;

                let active_filter_dlg = lw_tables_multi_select.create(
                    $("#"+self.filter_dlg_container+"_search"),
                    $("#"+self.filter_dlg_container+"_sel"),
                    dlg_info =
                    {
                        count          : function () {return filt_ext.possible_values.length;},
                        get            : function (i) {return filt_ext.possible_values[i];},
                        visible        : function (i) {
                                            if (dlg_info.cur_regexp == undefined)
                                                return true;
                                            let s = filt_ext.possible_values[i];
                                            let n = s.search(dlg_info.cur_regexp);
                                            if (n < 0) return false;
                                            return true;
                                        },
                        selected       : function (i) {
                                            return filt_ext.selected_values[i];       
                                        },
                        all_selected   : function () {
                                            let n = dlg_info.count();
                                            for(let i = 0; i < n;++i)
                                             if (dlg_info.visible(i) && !dlg_info.selected(i)) return false;
                                            return true;
                                        },
                        some_selected   : function () {
                                            let n = dlg_info.count();
                                            for(let i = 0; i < n;++i)
                                             if (dlg_info.visible(i) && dlg_info.selected(i)) return true;
                                            return false;
                                        },

                        title          : function (i) {return {en:filt_ext.possible_values[i],de:filt_ext.possible_values[i]};},
                        toggle         : function (i) {
                                            filt_ext.selected_values[i] = !filt_ext.selected_values[i];
                                            let v = [];
                                            for(let j = 0; j < filt_ext.selected_values.length;++j) 
                                             if (!filt_ext.selected_values[j]) v = v.concat(filt_ext.possible_values_ext[filt_ext.possible_values[j]]);
                                            //console.log(v);
                                            //ti.add_row_filter(col,v,filt_ext);
                                            ti.modify_filter(col_id,historize);   
                                            dlg_info.update_glyph();                                              
                                        },
                        select          : function(v,b){
                                            if (!v.length) return;
                                            for(let i = 0; i < v.length;++i){
                                                filt_ext.selected_values[v[i]] = b;
                                            }
                                            let w = [];
                                            for(let j = 0; j < filt_ext.selected_values.length;++j) 
                                             if (!filt_ext.selected_values[j]) w = w.concat(filt_ext.possible_values_ext[filt_ext.possible_values[j]]);
                                            //ti.add_row_filter(col,w,filt_ext);
                                            ti.modify_filter(col_id,historize);
                                            dlg_info.update_glyph();
                                        },
                        update_glyph : function(){
                                            let e = $('#lw-tables-th-default-glyph-filter-'+ti.tbl_id+'-'+col_id);                                            
                                            let col = ti.data_source.get_col_by_id(col_id);
                                            let active = ti.filter_active(col);
                                            if (active) $(e).removeClass("lw-tables-filter").removeClass("lw-tables-filter-on").addClass("lw-tables-filter-on");
                                            else $(e).removeClass("lw-tables-filter").removeClass("lw-tables-filter-on").addClass("lw-tables-filter");
                                        }
                    },
                    1,
                    {min_width:"100px",main_part_height:"300px"});
                window.onclick = function(ev){
                    let ofs = $("#"+self.filter_dlg_container).offset();let w = $("#"+self.filter_dlg_container).width();
                    let h = $("#"+self.filter_dlg_container).height();
                    
                    if (ev.clientX < ofs.left || ev.clientY < ofs.top || ev.clientX > w+ofs.left || ev.clientY > h+ofs.top ) self.close_filter_dlg(ev,tbl_id,col_id);
                };
        } /* case col type isstring */
    }
   };
})();


function build_table(lw_tables,container,v,cd){
    let vv = [];
    (function(){
    //let i = 0;
    let self = undefined;
    for(let h = 0; h < v.length;++h){
     let i = h;
     vv.push(
        self = {
            //v_copy : v,
            html: function(current_pos,tbl_info){

                

                let pin_class = 'lw-tables-pin';
                let sort_class = 'lw-tables-sort';
                let filter_class = 'lw-tables-filter';
                let sort_icon = "sort";

                if (current_pos < tbl_info.fixed_columns)
                 pin_class = 'lw-tables-unpin';
                if (tbl_info.filter_active(i))
                 filter_class = 'lw-tables-filter-on';          

                if (tbl_info.sort_active(i) == lw_tables.SORT_ASCENDING)
                 {sort_class = 'lw-tables-sort-ascending';sort_icon = "sort-by-attributes";}
                else if (tbl_info.sort_active(i) == lw_tables.SORT_DESCENDING)
                 {sort_class = 'lw-tables-sort-descending';sort_icon = "sort-by-attributes-alt";}

                let s = '<table style="table-layout:fixed;width:100%;height:95%;">'+
                '<tr style="height:20px;">'+
                '<td style="border:none;overflow:hidden;text-align:right;padding-top:5px;">'+

                '<div style="float:right;">'+
                 '<button '+
                           ' id="lw-tables-th-default-btn-sort-'+tbl_info.tbl_id+'-'+i+'"'+
                           ' onclick="'+
                           '  lw_tables.toggle_sort(event,'+tbl_info.tbl_id+','+i+');'+
                           ' " '+
                           ' style= "outline:none;background-color:transparent;"'+
                           ' type="button" class="'+sort_class+'  btn btn-default btn-xs" aria-label="Left Align">'+
                 '<span '+
                  'id="lw-tables-th-default-glyph-sort-'+tbl_info.tbl_id+'-'+i+'" '+
                  'class="glyphicon glyphicon-'+sort_icon+' small" style="xpadding:2px;" aria-hidden="true"></span>'+
               '</button></div>'+

                
                '<div class="" style="float:right;" >'+
                 '<button '+
                   'id="lw-tables-th-default-glyph-filter-'+tbl_info.tbl_id+'-'+i+'" '+
                   'onclick="lw_tables.show_filter_dlg(event,'+tbl_info.tbl_id+','+i+');"'+
                   'style="border:none;outline:none;background-color:transparent;"'+
                   'class="btn btn-default btn-xs dropdown-toggle '+filter_class+'" type="button" data-toggle="dropdown">'+
                   '<span '+
                   
                   ' style="xpadding:2px;" class="glyphicon glyphicon-filter"></span>'+
                   //'<span class="caret"></span>'+
                 '</button>'+
                '</div>'+

                '<div style="float:right;">'+
                 '<button '+
                 ' id="lw-tables-th-default-btn-pin-'+tbl_info.tbl_id+'-'+i+'"'+
                  'style="outline:none; background-color:transparent;" '+
                  'onclick="'+
                  'lw_tables.toggle_pin('+tbl_info.tbl_id+','+i+');'+
                  '" type="button" class="'+pin_class+'  btn btn-default btn-xs" aria-label="Left Align">'+
                 '<i class="fas fa-thumbtack" style="font-size:12px;"></i>'+
                '</button></div>'+

                '</td>'+
                '</tr>'+
                '<tr>'+
                '<td style="vertical-align:top;border:none;text-align:left;overflow:hidden;width:75%;padding-left:10px;" '+
                     'draggable="true" '+
                     'ondragover="lw_tables.allow_drop(event,'+current_pos+','+tbl_info.tbl_id+');" '+
                     'ondragstart="lw_tables.start_drag(event,'+current_pos+','+tbl_info.tbl_id+');"'+
                     'ondragleave="lw_tables.drag_leave(event,'+current_pos+','+tbl_info.tbl_id+');"'+
                     'ondrop="lw_tables.drop(event,'+current_pos+','+tbl_info.tbl_id+');"'+
                '>'+
                '<span class = "lw-tables-caption">'+
                    build_lang_aware_text_elem(v[i].title,language_context.current)+
                '</span>'+'</td>'+
                '</tr>'+
                '</table>';
                return s;
            },
            id : i,
            compare_fct : function(ordering){
                if (ordering == lw_tables.SORT_ASCENDING) return function (a,b) { if (a==b) return 0; if (a < b) return -1; return 1;};
                else return function (a,b) {if (a==b) return 0; if (a < b) return 1; return -1;};
            }
       }
     );}})();
    //console.log(cd);
    let tbl_id = lw_tables.create_table(lw_tables.create_data_source_from_matrix(cd,vv));
    let tbl_info = lw_tables.get_table(tbl_id);
    tbl_info.number_of_rows = function () {return tbl_info.data_source.row_count();};

       
    let wi = tbl_info.col_w;
    let hdr_dummy_column_width = tbl_info.hdr_dummy_column_width;


    for(let i = 0; i < tbl_info.data_source.col_count(); ++i) wi.push(100);

    function compute_available_height(){
        let container_element = document.getElementById(container);
        let container_boundingrect = container_element.getBoundingClientRect();
        return /*document.documentElement.clientHeight-container_boundingrect.top*/container_boundingrect.height;
    }
    function compute_available_width(){
        return $("#"+container).width()+30;
    }
    function width_of_first_n_cols(n){
        let r = 0;
        for(let i = 0; i < n;++i) r+= wi[i];
        return r;
    }


    let w_left = width_of_first_n_cols(tbl_info.fixed_columns);
    let tbl_left_pos = -15;
    let hdr_body_padding = 0;
    let hdr_right_right_padding = 4;

    ///let available_width = document.documentElement.clientWidth;
    let available_width = compute_available_width();
    let available_height = compute_available_height();
    let tbl_height = available_height;
    let tbl_header_height = 70;
    let tbl_body_left_height = tbl_height - tbl_header_height - hdr_body_padding;
    let tbl_body_right_height = tbl_body_left_height;
    
    let tbl_width_left  = w_left;
    let tbl_width_right  = available_width - tbl_width_left;
    tbl_info.display_left_width = tbl_width_left;
    tbl_info.display_right_width = tbl_width_right;
    tbl_info.tbl_header_height = tbl_header_height;
    tbl_info.tbl_left_pos = tbl_left_pos;



    let hdr =  tbl_info.html_gen.tbl_hdr(0,tbl_info.fixed_columns);
    let hdr_rest = tbl_info.html_gen.tbl_hdr(tbl_info.fixed_columns);

    let tbl_right = tbl_info.html_gen.tbl_body(undefined,undefined,tbl_info.fixed_columns);

    let tbl_left =  tbl_info.html_gen.tbl_body(undefined,undefined,0,tbl_info.fixed_columns);

    let content =
    '<div '+
        ' id="lw-tables-hdr-outer-left-'+tbl_id+'"'+
        ' style="'+
                'position:absolute;'+
                'overflow:hidden;'+
                'width:'+tbl_width_left+'px;'+
                'height:'+tbl_header_height+'px;'+
                'left:'+tbl_left_pos+'px;" > '+
     '<div '+
           ' id="lw-tables-hdr-inner-left-'+tbl_id+'"'+

           ' style= "position:absolute;overflow:hidden;" >'+
      hdr+
     '</div>'+
    '</div>'+
    '<div '+
        ' id    = "lw-tables-hdr-outer-right-'+tbl_id+'"'+
        ' style = "'+
                  'position:absolute;'+
                  'overflow:hidden;'+
                  'width:'+(tbl_width_right)+'px;'+
                  'height:'+(tbl_header_height)+'px;'+
                  'left:'+(tbl_width_left+tbl_left_pos)+'px;" '+
         '>' + 
     '<div '+
           ' id="lw-tables-hdr-inner-right-'+tbl_id+'"'+
           ' style= "position:absolute;overflow:hidden;" >' + hdr_rest +
     '</div>' +
    '</div>'+

    '<div  id="lw-tables-data-outer-left-'+tbl_id+'" '+
           'style = "position:absolute;'+
                    'top:'+(tbl_header_height+hdr_body_padding)+'px;'+
                    'left:'+(tbl_left_pos)+'px;width:'+tbl_width_left+'px;'+
                    'height:'+tbl_body_left_height+'px;'+
                    //'height:100%;'+
                    'overflow:hidden;" >'+ 
        '<div '+
            ' style="position:absolute;"'+
            ' id="lw-tables-data-inner-left-'+tbl_id+'"> ' + 
                tbl_left + 
        '</div>' +
    '</div>'+
    
    '<div  id="lw-tables-data-outer-right-'+tbl_id+'"'+ 
         ' style = "'+
                   'position:absolute;'+
                   'top:'+(tbl_header_height+hdr_body_padding)+'px;'+
                   'left:'+(w_left+tbl_left_pos)+'px;'+
                   'width:'+tbl_width_right+'px;'+
                   'height:'+tbl_body_right_height+'px;'+
                   //'height:100%;'
                   //'background-color:red;'+
                   'overflow:hidden;overflow-y:scroll;overflow-x:scroll;" >'+ 
    '<div '+
          ' style = "position:absolute;"' +
          ' id="lw-tables-data-inner-right-'+tbl_id+'"'+
    '> ' + 
     tbl_right + '</div>' +
    '</div>';
     
    $("#"+container).html(content);

    $(window).resize(tbl_info.fit_layout = function(e){
        if (tbl_info.ignore_resize_event()) return;
        available_width =  compute_available_width();
        tbl_width_left  = w_left = tbl_info.display_left_width;
        tbl_info.display_right_width = tbl_width_right  = available_width - tbl_width_left - hdr_right_right_padding;

        $('#lw-tables-data-outer-right-'+tbl_id).css("width",tbl_width_right);
        $('#lw-tables-hdr-outer-right-'+tbl_id).css("width",tbl_width_right);

        available_height = compute_available_height();
        tbl_height = available_height;
        tbl_body_left_height = tbl_height - tbl_header_height - hdr_body_padding;
        tbl_body_right_height = tbl_body_left_height;
        $('#lw-tables-data-outer-left-'+tbl_id).css("height",tbl_body_left_height);
        $('#lw-tables-data-outer-right-'+tbl_id).css("height",tbl_body_right_height);
    });

    $('#lw-tables-data-outer-right-'+tbl_id).scroll(function(e){
     let x = document.getElementById('lw-tables-data-outer-right-'+tbl_id).scrollLeft;
     let y = document.getElementById('lw-tables-data-outer-right-'+tbl_id).scrollTop;
     
     document.getElementById('lw-tables-hdr-outer-right-'+tbl_id).scrollLeft = x;
     document.getElementById('lw-tables-data-outer-left-'+tbl_id).scrollTop = y;
    });
    return tbl_info;
}
