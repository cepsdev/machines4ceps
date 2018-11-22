const TILE_STATUS_OK    = 0;
const TILE_STATUS_DONE  = 1;
const TILE_STATUS_ERROR = 2;
const TILE_STATUS_WARN  = 3;


let rollaut_infobox = function (parent,tile_idx, data) {
 
  let THIS = {
    initialized : false,
    visible     : false,
    data        : data,
    tile_idx    : tile_idx,
    dom         : undefined,
    dom_cache   : {
      header : {
        main        : undefined,
        middle_text : undefined,
        middle_main : undefined,
        bottom_text : undefined,
        bottom_main : undefined
      },
      main : {
        outer                 : undefined,
        three_steps_tbl       : undefined,
        three_steps_tbl_outer : undefined,
        three_steps_steps     : [],
        three_steps_left_side : []
      }
    },
    layout      : {
      get_width_css         : function() {return "500px;";},
      get_height_css        : function() {return "300px;";},
      get_header_height_css : function() {return "100px";},
      get_three_steps_view_height_css : function() {return "180px";},
      get_three_steps_step_height_css : function() {return "60px";},
      get_three_steps_outer_div_step_height_css : function() {return "56px";},
      get_three_steps_outer_div_base_css : function() {return `padding-left:4px;padding-right:4px;`;},
      get_three_steps_step_height : function() {return 60;},
      get_three_steps_view_width_css : function() {return "480px";}
    },
    get_associated_tile_idx : function () {return THIS.tile_idx;},
    hide : function(){
     if (THIS.visible){
       if(THIS.dom!=undefined)THIS.dom.style.visibility = "hidden"; //THIS.dom.style.display = "none"; //THIS.dom.style.visibility = "hidden";
     }
     THIS.visible = false;
    },
    get_header_class        : function (){
      if (THIS.tile_idx == undefined || THIS.data == undefined) return "bg-secondary";
      if (THIS.data.info.status[THIS.tile_idx] == TILE_STATUS_OK) return "bg-primary";
      if (THIS.data.info.status[THIS.tile_idx] == TILE_STATUS_WARN) return "bg-warning";
      if (THIS.data.info.status[THIS.tile_idx] == TILE_STATUS_ERROR) return "bg-danger";
      return "bg-success";      
    },
    update_coverage_info    : function() {
      if (THIS.dom_cache.header.bottom_text!=undefined){   
       THIS.dom_cache.header.bottom_main.removeChild (THIS.dom_cache.header.bottom_text);     
       THIS.dom_cache.header.bottom_main.appendChild(
         THIS.dom_cache.header.bottom_text=document.createTextNode(`${Math.trunc(THIS.data.info.cov[THIS.tile_idx]*10000.0)/100}%`));        
      }
    },
    update_status_info      : function(){
     if(THIS.dom_cache.header.main)
      THIS.dom_cache.header.main.setAttribute("class",THIS.get_header_class());
    },
    update_title_info    : function() {
      if (THIS.dom_cache.header.middle_text!=undefined){   
       THIS.dom_cache.header.middle_main.removeChild (THIS.dom_cache.header.middle_text);     
       THIS.dom_cache.header.middle_main.appendChild(
         THIS.dom_cache.header.middle_text=document.createTextNode(`${THIS.data.info.title[THIS.tile_idx]}`));        
      }
    },
    update_steps_info : function() {
     THIS.three_steps_view.highlight_current_step();
    },
    update                  : function(){
      THIS.update_coverage_info();
      THIS.update_status_info();
      THIS.update_title_info();
      THIS.update_steps_info();
    },
    three_steps_view:{
     highlight_current_step:function(){
       let outer = THIS.dom_cache.main.three_steps_tbl_outer;
       let current_state = THIS.data.info.state[THIS.tile_idx];
       let labels = THIS.data.info.state_labels(THIS.tile_idx);
       let steps = THIS.dom_cache.main.three_steps_steps;
       let left_side = THIS.dom_cache.main.three_steps_left_side;
       
       for(let i = 0; i != steps.length;++i) 
       { 
         left_side[i].setAttribute("class","");
         steps[i].setAttribute("style",`${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;`);               
       }
       if (current_state-1 >= 0) steps[current_state-1].setAttribute("style",`${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;filter:opacity(50%) blur(0.6px);`);
       if (current_state+1 < steps.length ) steps[current_state+1].setAttribute("style",`${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;filter:opacity(50%) blur(0.6px);`);
       steps[current_state].setAttribute("style",
        `${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;border-top:1px solid;border-bottom:1px solid;font-weight: bold;`);     
       if (current_state+1 !=  steps.length) left_side[current_state].setAttribute("class","loading");
       let offset = -current_state*THIS.layout.get_three_steps_step_height()+THIS.layout.get_three_steps_step_height();
       outer.setAttribute("style",`position:relative;top:${offset}px;`);
     }
    },
    build_steps_view        : function() {      
     //console.log(THIS.data.info.state[THIS.tile_idx]);
     let info_1 = document.createElement("div");
     info_1.setAttribute("style","height:1em;");
     let main = document.createElement("div");
     main.setAttribute("style","");
     THIS.dom_cache.main.outer = main;

     let list_div = document.createElement("div");
     list_div.setAttribute("style",`overflow:hidden;margin:0 auto;height:${THIS.layout.get_three_steps_view_height_css()};width:${THIS.layout.get_three_steps_view_width_css()};`);
     let tbl = document.createElement("table");

     let tbl_outer = document.createElement("div");
     tbl_outer.setAttribute("style","position:relative;");
     tbl_outer.appendChild(tbl);
     THIS.dom_cache.main.three_steps_tbl = tbl;
     THIS.dom_cache.main.three_steps_tbl_outer =  tbl_outer;
     tbl.setAttribute("style",`table-layout: fixed;text-align:center;vertical-align:middle;width:100%;overflow:hidden;`);

     function build_steps_list(tbl){
       let labels = THIS.data.info.state_labels(THIS.tile_idx);
       THIS.dom_cache.main.three_steps_steps = [];
       for(let i = 0; i != labels.length;++i){
         let tr=document.createElement("tr");
         let td_left=document.createElement("td");
         let td_right=document.createElement("td");
         let td=document.createElement("td");
         td.setAttribute("style",`height:${THIS.layout.get_three_steps_step_height_css()};overflow:hidden;`);
         td_left.setAttribute("style",`text-align:center;vertical-align:middle;width:80px;overflow:hidden;`);
         td_right.setAttribute("style",`text-align:center;vertical-align:middle;width:80px;overflow:hidden;`);
         
         let tdc = document.createTextNode(labels[i]);
         let tdc_outer = document.createElement("div");
         tdc_outer.setAttribute("style",`${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;`);
         
         
         let left_side = document.createElement("div");
         let right_side = document.createElement("div");
         //loader.setAttribute("class","rollaut_connection_loader_medium");
         left_side.setAttribute("style","float:right;");

         td_left.appendChild(left_side);
         td_right.appendChild(right_side);
         tdc_outer.appendChild(tdc);

         THIS.dom_cache.main.three_steps_steps.push(tdc_outer);
         THIS.dom_cache.main.three_steps_left_side.push(left_side);
         td.appendChild(tdc_outer);
         tr.appendChild(td_left);
         tr.appendChild(td);
         tr.appendChild(td_right);
         tbl.appendChild(tr);
       }        
     }

     build_steps_list(tbl);
     setTimeout(THIS.three_steps_view.highlight_current_step,0); 

     list_div.appendChild(tbl_outer);
     main.appendChild(list_div);
     THIS.dom.appendChild(info_1);
     THIS.dom.appendChild(main);
    },
    build_dom               : function() {
      if (parent == undefined) parent = document.body;
      if (THIS.tile_idx == undefined) return;
       let outer_div = document.createElement("div");        
       outer_div.setAttribute("style",
                              `position:absolute;width:${THIS.layout.get_width_css()};height:${THIS.layout.get_height_css()};`+
                              `border-radius:2px;background-color:#F8F8FF;box-shadow: 1px 1px 2px rgba(0, 0, 0, .1);`
       );
       let header_div = document.createElement("div");
       THIS.dom_cache.header.main = header_div;
       header_div.setAttribute("style",`position:relative;left:0px;top:0px;height:${THIS.layout.get_header_height_css()};width:100%;`);
       header_div.setAttribute("class",THIS.get_header_class());
       let header_sub_div_top = document.createElement("div");
       header_sub_div_top.setAttribute("style","height:2em;");
       let header_sub_div_middle = document.createElement("div");
       header_sub_div_middle.setAttribute("style","color:#F8F8FF;padding-left:12px;");
       let header_sub_div_bottom = document.createElement("div");
       header_sub_div_bottom.setAttribute("style","color:#F8F8FF;padding-right:12px;margin-bottom:10px;text-align:right;");

       header_div.appendChild(header_sub_div_top);header_div.appendChild(header_sub_div_middle);header_div.appendChild(header_sub_div_bottom);
       THIS.dom_cache.header.middle_text = document.createTextNode(THIS.data.info.title[THIS.tile_idx]);
       let middle_h3 = document.createElement("h3");
       middle_h3.appendChild(THIS.dom_cache.header.middle_text);
       header_sub_div_middle.appendChild(middle_h3);
       THIS.dom_cache.header.middle_main = middle_h3;
       THIS.dom_cache.header.bottom_text = document.createTextNode(`${Math.trunc(THIS.data.info.cov[THIS.tile_idx]*10000.0)/100}%`);
       let h5_elem = document.createElement("h5");
       h5_elem.appendChild(THIS.dom_cache.header.bottom_text);
       THIS.dom_cache.header.bottom_main = h5_elem;
       header_sub_div_bottom.appendChild(h5_elem);
       outer_div.appendChild(header_div);
       parent.appendChild(THIS.dom = outer_div);
       THIS.build_steps_view();
       THIS.initialized = THIS.visible=true;
    },
    show : function () {
      if (!THIS.initialized) THIS.build_dom();
      if (!THIS.initialized) return;
      if (THIS.visible) return;
      THIS.visible = true;
      THIS.dom.style.visibility = "visible"; 
    },
    set_tile_idx : function (ti){
      THIS.tile_idx = ti;
    },
    set_position: function (x,y){
     if (!THIS.initialized) return;
     THIS.dom.style.left = x+"px";
     THIS.dom.style.top = y+"px";       
    }
  };
  return THIS;
 };


let ceps_tiles_component = function (parent, data) {
 let THIS = {
    tile_width                        : 200,
    tile_height_em                    : 3.0,
    progress_bar_height_em            : 0.5,
    dom_cache                         : [],
    tileidx2dom_slot                  : [],
    data                              : data,
    mm_handler                        : undefined,
    cov_changed                       : undefined,
    tile_status_changed               : undefined,
    state_changed                     : undefined,
    register_move_over_tile_handler   : function (f) {THIS.mm_handler = f;},
    register_coverage_changed_handler : function (f) {THIS.cov_changed = f;},
    register_status_changed_handler   : function (f) {THIS.tile_status_changed = f;},
    register_state_changed_handler    : function (f) {THIS.state_changed = f;},

    tiles_dom_slots          : {
     columns : [TILE_STATUS_OK,TILE_STATUS_WARN,TILE_STATUS_ERROR,TILE_STATUS_DONE],      
     slots   : [new Array(data.size),new Array(data.size),new Array(data.size),new Array(data.size)]
    },

    get_progress_bar_height_css      : function(tile_idx) {
     return THIS.progress_bar_height_em+"em";
    },

    get_tile_height_css      : function(tile_idx) {
     return THIS.tile_height_em+"em";
    },

    get_progress_bar_bootstrap_class : function (status){
     if (status == TILE_STATUS_OK) return "bg-primary";
     if (status == TILE_STATUS_WARN) return "bg-warning";
     if (status == TILE_STATUS_ERROR) return "bg-danger";
     return "bg-success";      
    },

    build_progress_indicator : function(tile_idx){
     let outer = document.createElement("div");
     let inner = document.createElement("div");
     THIS.dom_cache[tile_idx].progress_bar = inner;
     inner.setAttribute("class",THIS.get_progress_bar_bootstrap_class(THIS.data.info.status[tile_idx]));
     inner.setAttribute("style",
      `width:${Math.floor(THIS.data.info.cov[tile_idx]*100)}%;`+
      `border-radius:4px;`+
      `height:${THIS.get_progress_bar_height_css(tile_idx)};`);
     outer.setAttribute("style",`padding:3px;`);
     outer.appendChild(inner);
     return outer;      
    },

    update_progress_indicator_when_status_changes:function(tile_idx){
     let inner = THIS.dom_cache[tile_idx].progress_bar;
     if (inner == undefined) return;
     inner.setAttribute("class",THIS.get_progress_bar_bootstrap_class(THIS.data.info.status[tile_idx]));
    },

    build_header : function(tile_idx){
     let outer = document.createElement("div");
     let tbl = document.createElement("table");
     tbl.setAttribute("style",`table-layout: fixed;width:100%;`);
     let tr = document.createElement("tr");
     let td1 = document.createElement("td");
     td1.setAttribute("style",`text-align: left;width:70%;overflow:hidden;`);
     let td2 = document.createElement("td");
     THIS.dom_cache[tile_idx].percentage_information = td2;
     td2.setAttribute("style",`overflow:hidden;text-align: right;`);
     td2.appendChild(document.createTextNode(`${Math.floor(THIS.data.info.cov[tile_idx]*10000)/100}%`));
     tr.appendChild(td1);
     tr.appendChild(td2);
     tbl.appendChild(tr);
     let inner = document.createElement("small");
     inner.appendChild(document.createTextNode(THIS.data.info.title[tile_idx]));
     td1.appendChild(inner);
     outer.appendChild(tbl);
     return outer;      
    },

    build_tile_content : function(tile_idx) {
     let pi = THIS.build_progress_indicator(tile_idx);
     let header = THIS.build_header(tile_idx);
     let outer = document.createElement("div");
     outer.appendChild(header);outer.appendChild(pi);
     outer.setAttribute("style",`margin-right:4px;margin-bottom:4px;border-radius: 5px;box-shadow: 2px 2px 4px #aaaaaa;`);
     outer.setAttribute("class","rollaut_info_tile_minimized");
     return outer;
    },
    handler_mouse_leave : function (ev){
        if (THIS.mm_handler != undefined) THIS.mm_handler(undefined,ev.clientX,ev.clientY,ev); 
    },
    handler_mouse_move : function (ev){       
      for(let row_number = 0; row_number != THIS.data.size;++row_number){
       for(let k in THIS.tiles_dom_slots.columns){
        let col = k;
        k = THIS.tiles_dom_slots.columns[k];
        let r = THIS.tiles_dom_slots.slots[k][row_number].getBoundingClientRect();
        if (ev.clientX-r.left < 0) continue;
        if (ev.clientY-r.top < 0) continue; 
        if (ev.clientX-r.left > r.width) continue;
        if (ev.clientY-r.top > r.height) continue;         
        let tile_idx = THIS.data.get_tile_at_row(row_number,THIS.tiles_dom_slots.columns,k);
        if (THIS.mm_handler != undefined) THIS.mm_handler(tile_idx,ev.clientX,ev.clientY,ev); 
       
        return;
       }
      }
      if (THIS.mm_handler != undefined) THIS.mm_handler(undefined,ev.clientX,ev.clientY,ev); 
    },

    update : function(){
     for(let row_number = 0; row_number != THIS.data.size;++row_number){
       for(let k in THIS.tiles_dom_slots.columns){
        let col = k;
        k = THIS.tiles_dom_slots.columns[k];
        let tile_idx = THIS.data.get_tile_at_row(row_number,THIS.tiles_dom_slots.columns,k);
        if (tile_idx == undefined) continue;
        let dom = THIS.tiles_dom_slots.slots[k][row_number];
        dom.removeChild(dom.firstChild);
        dom.appendChild(THIS.build_tile_content(tile_idx));
       }
     }
    },
    set_state(tile_idx,new_state){      
      if (tile_idx == undefined) return;
      let old_state = THIS.data.info.state[tile_idx];
      THIS.data.info.state[tile_idx] = new_state;
      if(THIS.state_changed) THIS.state_changed(tile_idx,new_state,old_state);
    },
    set_coverage(tile_idx,cov){
     THIS.data.info.cov[tile_idx] = cov;
     if (THIS.cov_changed != undefined) THIS.cov_changed(tile_idx,cov);
     
     if (THIS.dom_cache.length > tile_idx && THIS.dom_cache[tile_idx].percentage_information != undefined ){
       THIS.dom_cache[tile_idx].percentage_information.removeChild(THIS.dom_cache[tile_idx].percentage_information.firstChild);
       THIS.dom_cache[tile_idx].percentage_information.appendChild(document.createTextNode(`${Math.floor(THIS.data.info.cov[tile_idx]*10000)/100}%`));
       THIS.dom_cache[tile_idx].progress_bar.style.width = `${Math.floor(THIS.data.info.cov[tile_idx]*100)}%`;
     }       
    },

    compute_tiles_in_current_order_of_given_status : function (status,visible){
     let v = [];
     for(let i = 0; i != THIS.data.ordering.length;++i){
       let ti = THIS.data.ordering[i];
       if (THIS.data.visibility[ti]!=visible) continue;
       if (THIS.data.info.status[ti] != status) continue;
       v.push(ti);
     }
     return v;
    },

    compute_number_of_tiles_in_current_order_of_given_status : function (status,visible){
     let r = 0;
     for(let i = 0; i != THIS.data.ordering.length;++i){
       let ti = THIS.data.ordering[i];
       if (THIS.data.visibility[ti]!=visible) continue;
       if (THIS.data.info.status[ti] != status) continue;
       ++r;
     }
     return r;
    },

    set_status(tile_idx,new_status){
     if (tile_idx >= THIS.data.info.status.length) return;
     if (new_status < 0 || new_status > 3) return;
     let current_status = THIS.data.info.status[tile_idx];
     if (current_status == new_status) return;
     
     if (!THIS.data.visibility[tile_idx]){
       THIS.data.info.status[tile_idx] == new_status;return;
     }
     //Step 1: Remove from column
     //Step 2: Move remaining tiles in column to fill hole
     //Step 3: Make place in destination column
     //Step 4: Insert in destination column
     
     let v = THIS.compute_tiles_in_current_order_of_given_status(current_status,true);
     THIS.data.info.status[tile_idx] = new_status;
     if(THIS.tile_status_changed) THIS.tile_status_changed(tile_idx,new_status,current_status);
     
     let first_after_removed_tile = 0;
     for(;first_after_removed_tile<v.length;++first_after_removed_tile){
       if (v[first_after_removed_tile] == tile_idx){
         ++first_after_removed_tile;break;
       }
     }

     for(let i = first_after_removed_tile; i < v.length;++i){
       --THIS.tileidx2dom_slot[v[i]];
     }

     //Step 1
     let r = THIS.tileidx2dom_slot[tile_idx];
     let p = THIS.tiles_dom_slots.slots[current_status][r];
     let tile_dom = p.firstChild;
     p.removeChild(tile_dom);
     
     //Step 2
     for(let i = r; i + 1 < THIS.tiles_dom_slots.slots[current_status].length; ++i){
       if (THIS.tiles_dom_slots.slots[current_status][i+1].firstChild == undefined) break;
       let t = THIS.tiles_dom_slots.slots[current_status][i+1].firstChild;
       THIS.tiles_dom_slots.slots[current_status][i+1].removeChild(t);
       THIS.tiles_dom_slots.slots[current_status][i].appendChild(t);
     }
     

     //Step 3:
     v = THIS.compute_tiles_in_current_order_of_given_status(new_status,true);
     let first_after_inserted_tile = 0;
     for(;first_after_inserted_tile<v.length;++first_after_inserted_tile){
       if (v[first_after_inserted_tile] == tile_idx){
         ++first_after_inserted_tile;
         break;
       }
     }
     let prev = THIS.tiles_dom_slots.slots[new_status][first_after_inserted_tile-1].firstChild;
     THIS.tileidx2dom_slot[tile_idx] = first_after_inserted_tile-1;
     if (prev != undefined) THIS.tiles_dom_slots.slots[new_status][first_after_inserted_tile-1].removeChild(prev);
     for(let i = first_after_inserted_tile; i < v.length;++i){
       let ti = v[i];
       let t = THIS.tiles_dom_slots.slots[new_status][i].firstChild;
       //if (first_after_inserted_tile + 1 == v.length) console.log("?",tile_idx,first_after_inserted_tile,v,"?");
       if (t != undefined) THIS.tiles_dom_slots.slots[new_status][i].removeChild(t);
       if (prev != undefined){
         THIS.tiles_dom_slots.slots[new_status][i].appendChild(prev);
       }
       prev = t;
       ++THIS.tileidx2dom_slot[ti];
     }
     //Step 4
     THIS.tiles_dom_slots.slots[new_status][first_after_inserted_tile-1].appendChild(tile_dom);

     THIS.update_progress_indicator_when_status_changes(tile_idx);
    },

    build_dom : function() {
     THIS.dom_cache = new Array(THIS.data.size);
     THIS.tileidx2dom_slot = new Array(THIS.data.size);

     for(let i = 0; i != THIS.dom_cache.length;++i) THIS.dom_cache[i] = {progress_bar: undefined, percentage_information: undefined};
     let table = document.createElement("table");
     table.addEventListener("mousemove",THIS.handler_mouse_move);
     table.addEventListener("mouseleave",THIS.handler_mouse_leave);
     table.setAttribute("style","table-layout: fixed;display: inline-block;overflow: hidden;");
     
     let tile_idx_previous = -1;
     let tile_idx = -1;

     for(let row_number = 0; row_number != THIS.data.size;++row_number){
      let row = document.createElement("tr");
      
      for(let k in THIS.tiles_dom_slots.columns){
       let col = k;
       k = THIS.tiles_dom_slots.columns[k];
       tile_idx = THIS.data.get_tile_at_row(row_number,THIS.tiles_dom_slots.columns,k);
       let td = document.createElement("td");
       td.setAttribute("style",`table-layout: fixed;width: ${THIS.tile_width}px;height: ${THIS.get_tile_height_css(row_number,col)};overflow:hidden;`);
       let content = document.createElement("div");
       //content.setAttribute("style","padding:8px;");
       
       if (tile_idx != undefined){
         if (THIS.data.info.status[tile_idx] == k){
          content.appendChild(THIS.build_tile_content(tile_idx)/*document.createTextNode(THIS.data.tiles_title[row_number])*/);
          THIS.tileidx2dom_slot[tile_idx] = row_number; 
         }
       }

       td.appendChild(content);
       THIS.tiles_dom_slots.slots[k][row_number] = td;
       row.appendChild(td);
      }
      table.appendChild(row);
     }
     parent.appendChild(table);
    }
 };
 THIS.build_dom();

 let info_box = rollaut_infobox(undefined,undefined,data);
 info_box.show();
 THIS.register_move_over_tile_handler(
   (tile_idx,x,y,ev) => {
     if (tile_idx == undefined){
       info_box.hide();
       return;
     }     
     info_box.set_tile_idx(tile_idx);
     info_box.show();
     info_box.set_position(ev.pageX+25,ev.pageY+25);
     info_box.update();
   }
 );
 THIS.register_coverage_changed_handler(    
   (tile_idx,cov) => {
     if (info_box.get_associated_tile_idx() != tile_idx) return;
     info_box.update_coverage_info();
   }
 );
 THIS.register_status_changed_handler((tile_idx,new_status,current_status)=>{
   if (info_box.get_associated_tile_idx() != tile_idx) return;
   info_box.update_status_info();
 });
 THIS.register_state_changed_handler((tile_idx,new_state,old_state)=>{
   if (info_box.get_associated_tile_idx() != tile_idx) return;
   info_box.update_steps_info();
 });



 return THIS; 
};
