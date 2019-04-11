/*

Copyright 2018,2019 Tomas Prerovsky <tomas.prerovsky@ceps.technology> 
and the RollAut project authors. 

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Google Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/





const TILE_STATUS_OK    = 0;
const TILE_STATUS_DONE  = 1;
const TILE_STATUS_ERROR = 2;
const TILE_STATUS_WARN  = 3;
const TILE_STATUS_INACTIVE  = 4;
const TILE_STATUS_MAX = 4;


let rollaut_infobox = function (parent,
                                tile_idx, 
                                data) { 
  let THIS = {
    initialized   : false,
    visible       : false,
    data          : data,
    tile_idx      : tile_idx,
    rendered_step : undefined,
    dom           : undefined,
    dom_cache     : {
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
        three_steps_left_side : [],
        three_steps_right_side : []
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
      function two_digits(d){
        if (d == 0) return "00";
        if (d < 10) return `0${d}`;
        return `${d}`;
      }

      
       let outer = THIS.dom_cache.main.three_steps_tbl_outer;
       let current_state = THIS.data.info.state[THIS.tile_idx];
       //console.log(current_state);
       let labels = THIS.data.info.state_labels(THIS.tile_idx);
       let steps = THIS.dom_cache.main.three_steps_steps;
       let left_side = THIS.dom_cache.main.three_steps_left_side;
       let right_side = THIS.dom_cache.main.three_steps_right_side;

       if (current_state-1 >= 0 && right_side[current_state-1] !=undefined && right_side[current_state-1].firstChild != undefined) 
        right_side[current_state-1].removeChild(right_side[current_state-1].firstChild);
       if (current_state-1 >= 0 && left_side[current_state-1] !=undefined && left_side[current_state-1].firstChild != undefined) 
        left_side[current_state-1].removeChild(left_side[current_state-1].firstChild);
       
       if (right_side[current_state] !=undefined && right_side[current_state].firstChild != undefined) 
        right_side[current_state].removeChild(right_side[current_state].firstChild);
       if (left_side[current_state] !=undefined && left_side[current_state].firstChild != undefined) 
        left_side[current_state].removeChild(left_side[current_state].firstChild);
       if (current_state+1 !=  steps.length && right_side[current_state+1] !=undefined && right_side[current_state+1].firstChild != undefined) 
        right_side[current_state+1].removeChild(right_side[current_state+1].firstChild);       
       if (current_state+1 !=  steps.length && left_side[current_state+1] !=undefined && left_side[current_state+1].firstChild != undefined) 
        left_side[current_state+1].removeChild(left_side[current_state+1].firstChild);

       if (true/*THIS.rendered_step != current_state*/) {
        for(let i = 0; i != steps.length;++i) 
        { 
          left_side[i].setAttribute("class",""); left_side[i].setAttribute("style",``);right_side[i].setAttribute("style",``);
          steps[i].setAttribute("style",`${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;`);               
        }

        if (current_state-1 >= 0) 
         steps[current_state-1].setAttribute("style",`${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;filter:opacity(50%) blur(0.6px);`);
        if (current_state+1 < steps.length ) steps[current_state+1].setAttribute("style",`${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;filter:opacity(50%) blur(0.6px);`);

        steps[current_state].setAttribute("style",
          `${THIS.layout.get_three_steps_outer_div_base_css()}
          height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;border-top:1px solid;border-bottom:1px solid;font-weight: bold;`);     

      
        if (THIS.data.up_since != null) {
          let enter_t = THIS.data.info.enter_time(THIS.tile_idx,current_state);
          let d1 = new Date(THIS.data.up_since*1000+enter_t.secs_since_uptime*1000+enter_t.msecs_since_uptime);
          if (current_state + 1 == steps.length)
          right_side[current_state].appendChild(
            document.createTextNode(`${two_digits(d1.getHours())}:${two_digits(d1.getMinutes())}:${two_digits(d1.getSeconds())}`));
          else 
           right_side[current_state].appendChild(
            document.createTextNode(`${two_digits(d1.getHours())}:${two_digits(d1.getMinutes())}:${two_digits(d1.getSeconds())}`+" -\n ..." ));
        }

        if (current_state+1 !=  steps.length) {
          left_side[current_state].setAttribute("class","loading");
          left_side[current_state].setAttribute("style",`text-align:left;margin-left:20px;`);
        }


        if (current_state-1 >= 0){ // If there is a preceding state
          if (left_side[current_state-1].firstChild != undefined) left_side[current_state-1].removeChild(left_side[current_state-1].firstChild);
          if (right_side[current_state-1].firstChild != undefined) right_side[current_state-1].removeChild(right_side[current_state-1].firstChild);

          left_side[current_state-1].setAttribute("style","text-align:left;padding-left:4px;font-size: small;filter:opacity(50%) blur(0.6px);");
          right_side[current_state-1].setAttribute("style","text-align:left;padding-left:4px;font-size: small;filter:opacity(50%) blur(0.6px);");

          let enter_t = THIS.data.info.enter_time(THIS.tile_idx,current_state-1);
          let exit_t = THIS.data.info.exit_time(THIS.tile_idx,current_state-1);
          
          if (exit_t != undefined && enter_t != undefined ){
            THIS.rendered_step = current_state;
            let d_msecs_tot = (exit_t.secs_since_uptime*1000+exit_t.msecs_since_uptime) - (enter_t.secs_since_uptime*1000+enter_t.msecs_since_uptime);
            let d_hours = Math.floor(d_msecs_tot / (60*60000) );
            let d_min = Math.floor(d_msecs_tot / 60000 );
            let d_secs = Math.floor(d_msecs_tot / 1000) % 60;
            let d_msecs = d_msecs_tot % 1000;
            left_side[current_state-1].appendChild(document.createTextNode(`${two_digits(d_hours)}:${two_digits(d_min)}:${two_digits(d_secs)}.${d_msecs}`+"\n(Duration)" ));

            let d1 = new Date(THIS.data.up_since*1000+enter_t.secs_since_uptime*1000+enter_t.msecs_since_uptime);
            let d2 = new Date(THIS.data.up_since*1000+exit_t.secs_since_uptime*1000+exit_t.msecs_since_uptime);

            right_side[current_state-1].appendChild(
              document.createTextNode(`${two_digits(d1.getHours())}:${two_digits(d1.getMinutes())}:${two_digits(d1.getSeconds())}`+"-\n"+
              `${two_digits(d2.getHours())}:${two_digits(d2.getMinutes())}:${two_digits(d2.getSeconds())}` ));


          } else {
            left_side[current_state-1].appendChild(document.createTextNode(`computing...`));
            if (enter_t != undefined)
            {
              let d1 = new Date(THIS.data.up_since*1000+enter_t.secs_since_uptime*1000+enter_t.msecs_since_uptime);
              right_side[current_state-1].appendChild(
                document.createTextNode(`${two_digits(d1.getHours())}:${two_digits(d1.getMinutes())}:${two_digits(d1.getSeconds())}`+"-\n"
                 ));
  

            }
          }

        }
       }
       let offset = -current_state*THIS.layout.get_three_steps_step_height()+THIS.layout.get_three_steps_step_height();       
       outer.setAttribute("style",`position:relative;top:${offset}px;`);
     }
    },

    build_steps_view        : function() {      
     THIS.rendered_step = undefined;
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
       THIS.dom_cache.main.three_steps_left_side = [];
       THIS.dom_cache.main.three_steps_right_side = [];


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
         tdc_outer.setAttribute("style",
          `${THIS.layout.get_three_steps_outer_div_base_css()}height:${THIS.layout.get_three_steps_outer_div_step_height_css()};overflow:hidden;`);
         let left_side = document.createElement("div");
         let right_side = document.createElement("div");
         //loader.setAttribute("class","rollaut_connection_loader_medium");
         left_side.setAttribute("style","float:right;");
         td_left.appendChild(left_side);
         td_right.appendChild(right_side);
         tdc_outer.appendChild(tdc);
         THIS.dom_cache.main.three_steps_steps.push(tdc_outer);
         THIS.dom_cache.main.three_steps_left_side.push(left_side);
         THIS.dom_cache.main.three_steps_right_side.push(right_side);
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
                              `z-index:1000000;`+
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
      let b = THIS.tile_idx != ti;
      THIS.tile_idx = ti;
      return b; 
    },
    set_position: function (x,y){
     if (!THIS.initialized) return;
     THIS.dom.style.left = x+"px";
     THIS.dom.style.top = y+"px";       
    }
  };
  return THIS;
 };






















let ceps_tiles_component = function (parent, data, style_info,info_box_info) {
 let tile_width_default = 200;
 if (style_info != null && style_info.tile_width != null)
  tile_width_default = style_info.tile_width;
 section_name = new Map();
 section_name.set(TILE_STATUS_ERROR, "Failed");
 section_name.set(TILE_STATUS_WARN  , "Critical");
 section_name.set(TILE_STATUS_OK    , "OK");
 section_name.set(TILE_STATUS_DONE,"Complete");
 section_name.set(TILE_STATUS_INACTIVE,"Ready");
 let THIS = {
    cols_per_row                      : 5,
    tile_width                        : tile_width_default,
    tile_height_em                    : 3.0,
    progress_bar_height_em            : 0.5,
    dom_cache                         : [],
    tileidx2dom_slot                  : [],
    data                              : data,
    mm_handler                        : undefined,
    mclick_handler                    : undefined,
    cov_changed                       : undefined,
    tile_status_changed               : undefined,
    state_changed                     : undefined,
    register_click_tile_handler       : function (f) {THIS.mclick_handler = f;},
    register_move_over_tile_handler   : function (f) {THIS.mm_handler = f;},
    register_coverage_changed_handler : function (f) {THIS.cov_changed = f;},
    register_status_changed_handler   : function (f) {THIS.tile_status_changed = f;},
    register_state_changed_handler    : function (f) {THIS.state_changed = f;},

    dom                               : [],
    tiles_dom_cache                   : [],

    section_name : section_name,
    tiles_dom_slots          : {
     columns  : [TILE_STATUS_OK,TILE_STATUS_WARN,TILE_STATUS_ERROR,TILE_STATUS_DONE],
     sections : [TILE_STATUS_ERROR,TILE_STATUS_WARN,TILE_STATUS_OK,TILE_STATUS_DONE,TILE_STATUS_INACTIVE],   
     slots    : [new Array(data.size),new Array(data.size),new Array(data.size),new Array(data.size)]
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

    set_progbar_color:function(tile_idx){
      let backcol="black";
      if(TILE_STATUS_INACTIVE == THIS.data.info.status[tile_idx]){
       backcol = "#6c757d";
      } else if(TILE_STATUS_OK == THIS.data.info.status[tile_idx]){
       backcol = "#007bff";
      } else if (TILE_STATUS_WARN == THIS.data.info.status[tile_idx]){
      backcol = "#ffc107";
      }  else if (TILE_STATUS_ERROR == THIS.data.info.status[tile_idx]){
       backcol = "#dc3545";
      } else if (TILE_STATUS_DONE == THIS.data.info.status[tile_idx]) {
       backcol = "#28a745";
      }
      THIS.dom_cache[tile_idx].progress_bar.style.backgroundColor=backcol;
    },

    build_progress_indicator : function(tile_idx){
     let outer = document.createElement("div");
     let inner = document.createElement("div");
     THIS.dom_cache[tile_idx].progress_bar = inner;
     inner.setAttribute("style",
      `width:${Math.floor(THIS.data.info.cov[tile_idx]*100)}%;`+
      `border-radius:4px;`+
      `height:${THIS.get_progress_bar_height_css(tile_idx)};`+
      ``);
     THIS.set_progbar_color(tile_idx);
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
     THIS.dom_cache[tile_idx].outer = outer;

     let tbl = document.createElement("table");
     tbl.setAttribute("style",`table-layout: fixed;width:100%;`);
     let tr = document.createElement("tr");
     let td1 = document.createElement("td");
     
     if(THIS.data.info.cov[tile_idx] != null)
      td1.setAttribute("style",`text-align: left;width:70%;overflow:hidden;padding-left:4px;`);
     else
      td1.setAttribute("style",`text-align: left;width:100%;overflow:hidden;padding-left:4px;font-size:120%;`);

     let td2 = document.createElement("td");
     THIS.dom_cache[tile_idx].percentage_information = td2;
     td2.setAttribute("style",`overflow:hidden;text-align: right;`);
     td2.appendChild(document.createTextNode(`${Math.floor(THIS.data.info.cov[tile_idx]*10000)/100}%`));
     tr.appendChild(td1);
     
     if(THIS.data.info.cov[tile_idx] != null) tr.appendChild(td2);
     
     tbl.appendChild(tr);
     let inner = document.createElement("small");
     THIS.dom_cache[tile_idx].title = document.createElement("div");
     inner.appendChild(
      THIS.dom_cache[tile_idx].title       
     );
     THIS.dom_cache[tile_idx].title.appendChild(document.createTextNode(THIS.data.info.title[tile_idx]));
     td1.appendChild(inner);
     outer.appendChild(tbl);
     return outer;      
    },

    set_tile_style: function (tile_idx){
      let outer = THIS.tiles_dom_cache[tile_idx];
      let backcol = "";
      if(TILE_STATUS_INACTIVE == THIS.data.info.status[tile_idx]){
        //backcol = "background-color:#6c757d38;";
        backcol = "background-color:#e8eaf6;";
      } else if(TILE_STATUS_OK == THIS.data.info.status[tile_idx]){
       backcol = "background-color:#007bff38;";
      } else if (TILE_STATUS_WARN == THIS.data.info.status[tile_idx]){
        backcol = "background-color:#ffc10738;";
      }  else if (TILE_STATUS_ERROR == THIS.data.info.status[tile_idx]){
        backcol = "background-color:#dc354538;";
      } else if (TILE_STATUS_DONE == THIS.data.info.status[tile_idx]) {
        backcol = "background-color:#28a74538;";
      }
      
      outer.setAttribute("style",`margin-right:4px;
      margin-bottom:4px;
      border-radius: 5px;
      box-shadow: 2px 2px 4px #aaaaaa;${backcol}`);

      THIS.set_progbar_color(tile_idx);
    },

    build_tile_content : function(tile_idx) {
     let pi = THIS.build_progress_indicator(tile_idx);
     let header = THIS.build_header(tile_idx);
     for (;THIS.tiles_dom_cache.length < tile_idx;) THIS.tiles_dom_cache.push(null); 

     let outer = document.createElement("div");
     THIS.tiles_dom_cache[tile_idx] = outer;
     THIS.set_tile_style(tile_idx);

     outer.appendChild(header);outer.appendChild(pi);

     outer.setAttribute("class","rollaut_info_tile_minimized");
     return outer;
    },
    handler_mouse_click : function(ev){
      if (THIS.mclick_handler == null) return;
      for(let sec = 0; sec != THIS.dom.length;++sec){
       for(let i = 0; i !=  THIS.dom[sec].tile_divs.length;++i){
        let div = THIS.dom[sec].tile_divs[i];
        if (div.firstChild == null) break;
        let r = div.getBoundingClientRect();
        if (ev.clientX-r.left < 0) continue;
        if (ev.clientY-r.top < 0) continue; 
        if (ev.clientX-r.left > r.width) continue;
        if (ev.clientY-r.top > r.height) continue; 
        //hit
        let order_idx = i;
        let group_index = sec;
        let tile_idx = 0;
        let jj = 0;
        //compute tile index
        for(let h = 0; h != THIS.data.ordering.length;++h){
          let ti = THIS.data.ordering[h];
          if (!THIS.data.visibility[ti]) continue;
          if (THIS.data.info.status[ti] != sec) continue;
          if (jj==order_idx){
            tile_idx = ti;
            break;
          }
          ++jj;                      
        }
        if(!THIS.data.info.active[tile_idx]) return;
        THIS.mclick_handler(tile_idx,
          {
            status      : THIS.data.info.status[tile_idx],
            state       : THIS.data.info.state[tile_idx],
            title       : THIS.data.info.title[tile_idx],
            cov         : THIS.data.info.cov[tile_idx],
            up_time     : THIS.data.up_since,
            enter_times : THIS.data.info.enter_times(tile_idx),
            exit_times  : THIS.data.info.exit_times(tile_idx)
          });
       }
      }
    },
    handler_mouse_leave : function (ev){
        if (THIS.mm_handler != undefined) THIS.mm_handler(undefined,ev.clientX,ev.clientY,ev); 
    },
    handler_mouse_move : function (ev){
      if (THIS.mm_handler == undefined || THIS.mm_handler == null) 
       return;
      for(let sec = 0; sec != THIS.dom.length;++sec){
        for(let i = 0; i !=  THIS.dom[sec].tile_divs.length;++i){
         let div = THIS.dom[sec].tile_divs[i];
         if (div.firstChild == null) break;
         let r = div.getBoundingClientRect();
         if (ev.clientX-r.left < 0) continue;
         if (ev.clientY-r.top < 0) continue; 
         if (ev.clientX-r.left > r.width) continue;
         if (ev.clientY-r.top > r.height) continue;         
         //hit
         let order_idx = i;
         let group_index = sec;
         let tile_idx = -1;
         let jj = 0;
         //compute tile index
         for(let h = 0; h != THIS.data.ordering.length;++h){
           let ti = THIS.data.ordering[h];
           if (!THIS.data.visibility[ti]) continue;
           if (THIS.data.info.status[ti] != sec) continue;
           if (jj==order_idx){
             tile_idx = ti;
             break;
           }
           ++jj;                      
         }
         if(!THIS.data.info.active[tile_idx]) return;
         THIS.mm_handler(tile_idx,ev.clientX,ev.clientY,ev); 
         return; 
        }
        //THIS.mm_handler(undefined,ev.clientX,ev.clientY,ev); 
      } 
      /*for(let row_number = 0; row_number != THIS.data.size;++row_number){
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
      if (THIS.mm_handler != undefined) THIS.mm_handler(undefined,ev.clientX,ev.clientY,ev); */
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
    apply_filter : function(f){
      for (let tile_idx = 0; tile_idx != THIS.data.visibility.length;++tile_idx){
        THIS.data.visibility[tile_idx] = f(tile_idx,THIS.data.info);
      }
      THIS.build_dom();
    },
    set_active:function(idx,b){
      let old_value = THIS.data.info.active[idx];
      if (old_value == b) return;
      THIS.data.info.active[idx] = b;
      if (!b){
        THIS.dom_cache[idx].outer.setAttribute("style",`color: grey;`);
        THIS.tiles_dom_cache[idx].setAttribute("class","rollaut_info_tile_minimized_deactive");
      } else {
        THIS.dom_cache[idx].outer.setAttribute("style",`color: black;`);
        THIS.tiles_dom_cache[idx].setAttribute("class","rollaut_info_tile_minimized");
      }
    },
    set_state(tile_idx,new_state){
      if (tile_idx == undefined) return;
      let old_state = THIS.data.info.state[tile_idx];
      THIS.data.info.state[tile_idx] = new_state;
      if(THIS.state_changed) THIS.state_changed(tile_idx,new_state,old_state);
    },
    set_title(tile_idx,t){
      THIS.data.info.title[tile_idx] = t;
      THIS.dom_cache[tile_idx].title.removeChild(THIS.dom_cache[tile_idx].title.firstChild);
      THIS.dom_cache[tile_idx].title.appendChild(document.createTextNode(t));
    },
    set_coverage(tile_idx,cov){
     THIS.data.info.cov[tile_idx] = cov;
     if (THIS.cov_changed != undefined) THIS.cov_changed(tile_idx,cov);
     
     if (THIS.dom_cache.length > tile_idx && THIS.dom_cache[tile_idx].percentage_information != undefined ){
       THIS.dom_cache[tile_idx].percentage_information.removeChild(THIS.dom_cache[tile_idx].percentage_information.firstChild);
       THIS.dom_cache[tile_idx].percentage_information.appendChild(document.createTextNode(`${Math.floor(THIS.data.info.cov[tile_idx]*10000)/100}%`));
       THIS.dom_cache[tile_idx].progress_bar.style.width = `${Math.floor(THIS.data.info.cov[tile_idx]*100)}%`;
       THIS.set_progbar_color(tile_idx);
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

    compute_index_of_entity_in_section:function (tile_idx,status){
      //Assume No sort enabled,TODO:Replace with general case
      let r = 0;
      for(let i = 0; THIS.data.ordering.length != i;++i){
        if(THIS.data.ordering[i] == tile_idx) return r;
        if( THIS.data.info.status[THIS.data.ordering[i]] != status) continue;
        if(!THIS.data.visibility[THIS.data.ordering[i]]) continue;
        ++r;
      }
      return null;
    },
    
    compute_visibility_of_entity:function (tile_idx){
      return true;
    },

    build_empty_row: function (sec,tr) {
      for(let col_index = 0;col_index != this.cols_per_row;++col_index){
        let td = document.createElement("td");
        td.setAttribute("valign","top");
        td.setAttribute("style",
         `table-layout: fixed;width: ${THIS.tile_width}px;
          overflow:hidden;`);
        let content = document.createElement("div");
        THIS.dom[sec].tile_divs.push(content);
        td.appendChild(content);
        tr.appendChild(td);
      }
    },

    remove_entity:function(tile_idx){
      if (tile_idx >= THIS.data.size) return;
      
      let status = THIS.data.info.status[tile_idx];
      let visibility = THIS.data.visibility[tile_idx];
      let order_idx = THIS.compute_index_of_entity_in_section(tile_idx,status);

      THIS.data.info.status.splice(tile_idx,1);
      THIS.data.info.state.splice(tile_idx,1);
      THIS.data.info.title.splice(tile_idx,1);
      THIS.data.info.active.splice(tile_idx,1);
      THIS.data.info.cov.splice(order_idx,1);
      THIS.data.ordering.splice(tile_idx,1);
      for(let i = 0; i!=THIS.data.ordering.length;++i){
        if (THIS.data.ordering[i] > tile_idx)
         --THIS.data.ordering[i];
      }
      THIS.data.visibility.splice(tile_idx,1);
      --THIS.data.size;

      THIS.dom_cache.splice(tile_idx,1);
      THIS.tiles_dom_cache.splice(tile_idx,1);

      if (!visibility) return;
      THIS.dom[status].tile_divs[order_idx].removeChild(THIS.dom[status].tile_divs[order_idx].firstChild);
      for(let i = order_idx; i+1 != THIS.dom[status].tile_divs.length;++i) {
        let right_tile = THIS.dom[status].tile_divs[i+1];
        let this_tile = THIS.dom[status].tile_divs[i];
        if (right_tile.firstChild == null) break;
        let tile_content = right_tile.firstChild;
        right_tile.removeChild(tile_content);
        this_tile.appendChild(tile_content);        
      }
      THIS.update_section_header(status,
        THIS.compute_number_of_tiles_in_current_order_of_given_status(status,true)>0); 
    },

    insert_entity: function (tile_idx,elem_data){
      
      THIS.data.info.status.splice(tile_idx,0,elem_data.status);
      THIS.data.info.state.splice(tile_idx,0,elem_data.state);
      THIS.data.info.title.splice(tile_idx,0,elem_data.title);
      THIS.data.info.cov.splice(tile_idx,0,elem_data.cov);
      THIS.data.info.active.splice(tile_idx,0,true);
      
      for(let i = 0; i!=THIS.data.ordering.length;++i){
        if (THIS.data.ordering[i] >= tile_idx)
         ++THIS.data.ordering[i];
      }

      THIS.data.ordering.splice(tile_idx,0,tile_idx);
      THIS.data.visibility.splice(tile_idx,0,THIS.compute_visibility_of_entity(tile_idx));
      let order_idx = THIS.compute_index_of_entity_in_section(tile_idx,elem_data.status);

        
      ++THIS.data.size;

      //Adjust DOM Cache
      THIS.dom_cache.splice(tile_idx,0,
        { progress_bar: undefined, 
        percentage_information: undefined});
      THIS.tiles_dom_cache.splice(tile_idx,0,null);

      if (!THIS.data.visibility[tile_idx]) return;

      let tile_content = THIS.build_tile_content(tile_idx);

      let destV = THIS.dom[elem_data.status].tile_divs;
      // INVARIANT destV.length > 0
      if (destV[destV.length-1].firstChild != null){
        //Out of space => Insert Row
        let tr = document.createElement("tr");
        THIS.build_empty_row(elem_data.status,tr);
        THIS.dom[elem_data.status].rows.push(tr);
        
        let display_order_of_sec = 0;
        for(;THIS.tiles_dom_slots.sections.length > display_order_of_sec;++display_order_of_sec){
          if (THIS.tiles_dom_slots.sections[display_order_of_sec] == elem_data.status) break;
        }

        if (display_order_of_sec + 1 < THIS.tiles_dom_slots.sections.length){
          let following_sec = THIS.tiles_dom_slots.sections[display_order_of_sec+1];
          THIS.dom[following_sec].header.parentNode.parentNode.parentNode.insertBefore(
            tr,
            THIS.dom[following_sec].header.parentNode.parentNode);
                              
        } else {
          THIS.dom[elem_data.status].header.parentNode.parentNode.parentNode.appendChild(tr);                    
        }
      }
      for(let i = order_idx; i < destV.length && tile_content != null;++i){
       let container = destV[i];
       let content = container.firstChild;
       if (content != null) container.removeChild(content);
       container.appendChild(tile_content);
       tile_content = content;
      }
      THIS.update_section_header(elem_data.status,true);     
    },

    set_status(tile_idx,new_status){
    //console.log(THIS.dom);

     if (tile_idx >= THIS.data.info.status.length) return;
     if (new_status < 0 || new_status > TILE_STATUS_MAX) return;
     let current_status = THIS.data.info.status[tile_idx];
     if (current_status == new_status) return;
     
     if (!THIS.data.visibility[tile_idx]){
       THIS.data.info.status[tile_idx] == new_status;return;
     }

     //Step 1: Remove from column
     //Step 2: Move remaining tiles in column to fill hole
     //Step 3: Make place in destination column
     //Step 4: Insert in destination column

     //1 && 2

     let v = THIS.compute_tiles_in_current_order_of_given_status(current_status,true);
     let tile_idx_in_section = -1;
     for(tile_idx_in_section = 0; v.length > tile_idx_in_section; ++tile_idx_in_section)
      if (v[tile_idx_in_section] == tile_idx) break;

     if (tile_idx_in_section >= THIS.dom[current_status].tile_divs.length) 
      console.log(tile_idx_in_section,"tile_idx_in_section >= THIS.dom[current_status].tile_divs.length");
     if (THIS.dom[current_status].tile_divs[tile_idx_in_section] == null) 
      console.log(tile_idx_in_section,"THIS.dom[current_status].tile_divs[tile_idx_in_section] == null");

     let tile_content = THIS.dom[current_status].tile_divs[tile_idx_in_section].firstChild;

     if (tile_content == null) 
      console.log(tile_idx_in_section,"tile_content == null");

     //console.log(tile_content.parent);

     tile_content.parentNode.removeChild(tile_content);
     //INVARIANT: THIS.dom[current_status].tile_divs[tile_idx_in_section] has no children
     for(let i = tile_idx_in_section; i + 1 < THIS.dom[current_status].tile_divs.length;++i){
       let e = THIS.dom[current_status].tile_divs[i+1].firstChild;
       if (e == null) break;
       e.parentNode.removeChild(e);
       THIS.dom[current_status].tile_divs[i].appendChild(e);
     }

     //3 && 4
     THIS.data.info.status[tile_idx] = new_status;
     let w = THIS.compute_tiles_in_current_order_of_given_status(new_status,true);
     let newTileIdx = 0;
     for(;newTileIdx != w.length;++newTileIdx)
      if (w[newTileIdx] == tile_idx) break;
     let destV = THIS.dom[new_status].tile_divs;
     if (destV.length < w.length)
     {
        //Out of space => Insert Row         
        let tr = document.createElement("tr");
        THIS.build_empty_row(new_status,tr);
        THIS.dom[new_status].rows.push(tr);
        
        let display_order_of_sec = 0;
        for(;THIS.tiles_dom_slots.sections.length > display_order_of_sec;++display_order_of_sec){
          if (THIS.tiles_dom_slots.sections[display_order_of_sec] == new_status) break;
        }
        if (display_order_of_sec + 1 < THIS.tiles_dom_slots.sections.length){
          let following_sec = THIS.tiles_dom_slots.sections[display_order_of_sec+1];
          THIS.dom[following_sec].header.parentNode.parentNode.parentNode.insertBefore(
            tr,
            THIS.dom[following_sec].header.parentNode.parentNode);
                              
        } else {
          THIS.dom[new_status].header.parentNode.parentNode.parentNode.appendChild(tr);                    
        }
        destV = THIS.dom[new_status].tile_divs;

     }
     for(let i = newTileIdx; i < destV.length && tile_content != null;++i){
       let container = destV[i];
       let content = container.firstChild;
       if (content != null) container.removeChild(content);
       container.appendChild(tile_content);
       tile_content = content;
     }

     
     THIS.data.info.status[tile_idx] = new_status;
     THIS.update_section_header(current_status,THIS.compute_number_of_tiles_in_current_order_of_given_status(current_status,true) > 0);
     THIS.update_section_header(new_status,THIS.compute_number_of_tiles_in_current_order_of_given_status(new_status,true) > 0);
     THIS.set_tile_style(tile_idx);
     


     if(Math.PI >= 3.0) return;
     
     v = THIS.compute_tiles_in_current_order_of_given_status(current_status,true);

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

    update_section_header : function(sec,visible){
      return;
      /*let sec_div = THIS.dom[sec].header;
      if (!visible) { sec_div.setAttribute("style","display:none;"); return}
      else sec_div.setAttribute("style","display:block;");

      sec_div.setAttribute("style","padding:2px;");

      if (sec == TILE_STATUS_OK)
       sec_div.setAttribute("class","alert alert-primary small");
      else if (sec == TILE_STATUS_DONE)
       sec_div.setAttribute("class","alert alert-success small");
      else if (sec == TILE_STATUS_ERROR)
       sec_div.setAttribute("class","alert alert-danger small");
      else if (sec == TILE_STATUS_WARN) 
       sec_div.setAttribute("class","alert alert-warning small");
      else
       sec_div.setAttribute("class","alert alert-secondary small");*/

    },

    build_dom : function() {

     for(;parent.firstChild != null;) parent.removeChild(parent.firstChild);
     
     let colsPerRow = THIS.cols_per_row;

     let tw = THIS.tile_width;

     THIS.dom_cache = new Array(THIS.data.size);
     THIS.tileidx2dom_slot = new Array(THIS.data.size);

     for(let i = 0; i != THIS.dom_cache.length;++i) THIS.dom_cache[i] = {
       progress_bar: undefined, 
       percentage_information: undefined
     };
     
     let table = document.createElement("table");
     table.addEventListener("mousemove",THIS.handler_mouse_move);
     table.addEventListener("mouseleave",THIS.handler_mouse_leave);
     table.addEventListener("click",THIS.handler_mouse_click);
     table.setAttribute("style","table-layout: fixed;display: inline-block;overflow: hidden;width:100%;");
     
     let tile_idx_previous = -1;
     let tile_idx = -1;

     THIS.dom = [];
     for(let i = 0; i!= TILE_STATUS_MAX+1;++i){
       THIS.dom.push(
         {
           empty     : true,
           header    : null,
           rows      : null,
           tile_divs : null
         }
       )
     }
     for (let i = 0; i != THIS.tiles_dom_slots.sections.length;++i){
       let sec = THIS.tiles_dom_slots.sections[i];
       let tiles = THIS.compute_tiles_in_current_order_of_given_status(sec,true);//THIS.data.get_tiles_with_status(sec);
       //console.log(tiles);
       //if (tiles.length == 0) continue;


       let header_row = document.createElement("tr");

       let tdh = document.createElement("td");
       tdh.setAttribute("colspan",colsPerRow);
       tdh.setAttribute("style",
                        `width:${colsPerRow*tw}px;
                         text-align:center;`);

      
      THIS.dom[sec].empty = tiles.length == 0;
      THIS.dom[sec].rows = [];
      THIS.dom[sec].tile_divs = [];
     
      let sec_div = THIS.dom[sec].header = document.createElement("div");
      THIS.update_section_header(sec,tiles.length != 0);

      //let sec_title = document.createTextNode(THIS.section_name.get(sec));
      //sec_div.appendChild(sec_title);
      
      tdh.appendChild(
         sec_div
       );
       header_row.appendChild(tdh);
       header_row.setAttribute("style","height:0px;");
       table.appendChild(header_row);
       let num_of_rows = Math.ceil(tiles.length / colsPerRow)+1;
       let cur_tile_idx = 0;
       for(let row_index = 0; row_index != num_of_rows; ++row_index){
         let row = document.createElement("tr");
         THIS.dom[sec].rows.push(row);
         for(let col_index = 0;col_index != colsPerRow;++col_index){
           let tile = tiles[cur_tile_idx];
           let td = document.createElement("td");
           td.setAttribute("valign","top");
           td.setAttribute("style",
            `table-layout: fixed;width: ${THIS.tile_width}px;
             overflow:hidden;`);
           let content = document.createElement("div");
           THIS.dom[sec].tile_divs.push(content);
           if (cur_tile_idx < tiles.length) {
             if (THIS.data.visibility[tile]) {
               content.appendChild(THIS.build_tile_content(tile));
             }
           }
           td.appendChild(content);
           row.appendChild(td);
           ++cur_tile_idx;
         }         
         table.appendChild(row);
       }
     }
     parent.appendChild(table);
    }
 };
 
 
 THIS.build_dom();

 if (info_box_info == null) return THIS;

 let info_box = rollaut_infobox(undefined,undefined,data);
 info_box.show();
 THIS.register_move_over_tile_handler(
   (tile_idx,x,y,ev) => {
     
     if (tile_idx == undefined){
       info_box.hide();
       return;
     }     
     let changed_tile = info_box.set_tile_idx(tile_idx);
     info_box.show();
     info_box.set_position(ev.pageX+25,ev.pageY+25);
     if(changed_tile) {
       info_box.update();
     }
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





function create_tiles_deck(
  parentWidget,
  dataModel,
  style_info,
  info_box) {

let data_size = dataModel.getSize();
let idx2sm = {};

let THIS = {
    cache             : {state_labels:{}},
    stateLabels       : dataModel.getStateLabels(),
    filters           : [],
    permutations      : [],
    size              : data_size,
    sm2idx            : [],
    enter_times       : {},
    exit_times        : {},
    tiles_widget      : null,
    up_since          : dataModel.getUpTime(),
    info              : {
      active      : new Array(data_size),
      status      : new Array(data_size),
      cov         : new Array(data_size),
      title       : new Array(data_size),
      state       : new Array(data_size), //a non negative integer
      state_labels : function (tile_idx){
        return THIS.stateLabels;
      },
      enter_time : function (tile_idx,state){
        return dataModel.getEnterTime(tile_idx,state);
      },
      exit_time : function (tile_idx,state){
        return dataModel.getExitTime(tile_idx,state);
      },       
      enter_times : function (tile_idx){
        return dataModel.getEnterTimes(tile_idx);
      },
      exit_times : function (tile_idx){
        return dataModel.getExitTimes(tile_idx);
      }
    },
    ordering    : new Array(data_size), // pos => tile index
    visibility  : new Array(data_size),
    compute_ordering  : function () {
      for(let i = 0; i != THIS.ordering.length;++i)
        THIS.ordering[i] = i;
    },
    compute_visibility  : function () {
      for(let i = 0; i != THIS.visibility.length;++i)
        THIS.visibility[i] = true;
    },
    get_tile_at_row : function(row,cols,col){
      let r = -1;
      for(let i = 0; i < THIS.ordering.length; ++i){
        if (!THIS.visibility[THIS.ordering[i]]) continue;
        if (THIS.info.status[[THIS.ordering[i]]] != col) continue;         
        ++r;
        if (r == row) return i;
      }
      return undefined;     
    },
    get_tiles_with_status: function (status_wanted){
      let r = [];
      for(let i = 0; i < THIS.ordering.length; ++i){
        if (!THIS.visibility[THIS.ordering[i]]) continue;
        if (THIS.info.status[[THIS.ordering[i]]] != status_wanted) continue;
        r.push(THIS.ordering[i]);
      }
      return r;
    },
    insert_entity: function (tile_idx,elem_data){
      THIS.tiles_widget.insert_entity(tile_idx,elem_data);
    },
    remove_entity:function(Index){
      THIS.tiles_widget.remove_entity(Index);
    },
    setActive:function(idx,b){
      THIS.tiles_widget.set_active(idx,b);
    },
    apply_filter:function(f){
      THIS.tiles_widget.apply_filter(f);
    }
  };

  let data = THIS;
  for(let i = 0; i    != data_size; ++i){
    data.info.state[i]  = dataModel.getState(i);
    data.info.status[i] = dataModel.getStatus(i);
    data.info.title[i]  = dataModel.getTitle(i);
    data.info.cov[i]    = dataModel.getCoverage(i);
    data.info.active[i] = true;
  }
  
  data.tile2states = dataModel.getTile2StatesMapping();
  data.tile2_visited_states = dataModel.getState2VisitedStates();
  data.state2root = dataModel.getState2RootMapping();
  data.state2label = dataModel.getState2LabelMapping();

  THIS.compute_ordering();
  THIS.compute_visibility();
  if (style_info == null)
    THIS.tiles_widget = ceps_tiles_component(
      parentWidget,
      data,
      {tile_width:300}
    );
  else {
    if (style_info["tile_width"] == null) style_info["tile_width"]=300;
    THIS.tiles_widget = ceps_tiles_component(
      parentWidget,
      data,
      style_info,
      info_box
    );
}
return THIS;
}
