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



let build_running_rollout_overview_dlg_update = null;
function kill_store(store,rollout_id){
   staccato_append_attribute("smcores/rollouts/scheduled/",rollout_id,"kill",store);
 }

 function restart_store(store,rollout_id){
   staccato_append_attribute("smcores/rollouts/scheduled/",rollout_id,"restart",store);
 }



 let update_running_rollout_overview_dlg_store_info_timeline_timer = null;
 function build_running_rollout_overview_dlg_store_info_timeline(body_div,status,cov,steps,store,tiles,tile_index){
  
  function get_time_info(step_idx){
    let outer = document.createElement("div");
    //outer.setAttribute("style","color:rgba(100,100,100,100);");
    let inner = document.createElement("h6");
    let t = "";
    let date = new Date(1000*(tiles.tiles_widget.data.info.enter_time(tile_index,step_idx).secs_since_uptime+store.up_time));
    t = date.toLocaleTimeString();
    inner.appendChild(document.createTextNode(t));
    outer.append(inner);
    return outer;
  };

  let tdls = [];
  let tdrs = [];
  
  function update_running_rollout_overview_dlg_store_info_timeline(){
    let cur_state = tiles.tiles_widget.data.info.state[tile_index];
    for(let i = 0; i != steps.length && i < cur_state;++i){
       tdrs[i].setAttribute("style","text-align:left;padding:4px;");
       if (tdls[i].firstChild != null)
        tdls[i].removeChild(tdls[i].firstChild);
       tdls[i].appendChild(get_time_info(i));
       tdls[i].setAttribute("style",
        `color:rgba(100,100,100,100);
         border-right: 2px solid;
         border-right-color: rgba(100,100,100,150);
         width: 100px;
         margin-top:10px;`);
    }

    tdrs[cur_state].setAttribute("style",
     `font-weight: bold;border: 2px solid grey;text-align:left;padding:4px;
      box-shadow: 2px 5px 4px #888888;
     `);
    
    if (tdls[cur_state].firstChild != null)
        tdls[cur_state].removeChild(tdls[cur_state].firstChild);
        tdls[cur_state].appendChild(get_time_info(cur_state));

    tdls[cur_state].setAttribute("style",
     `font-weight:bold;
      color:black;
      border-right: 2px solid;
      border-right-color: rgba(100,100,100,150);
      width: 100px;
      margin-top:10px;`);


    for(let i = cur_state + 1; i != steps.length;++i){
       tdrs[i].setAttribute("style","text-align:left;padding:4px;filter: opacity(0.4);");
       if (tdls[i].firstChild != null)
        tdls[i].removeChild(tdls[i].firstChild);
    }
  }

  if (update_running_rollout_overview_dlg_store_info_timeline_timer != null)
   clearInterval(update_running_rollout_overview_dlg_store_info_timeline_timer);
   update_running_rollout_overview_dlg_store_info_timeline_timer = 
    setInterval(update_running_rollout_overview_dlg_store_info_timeline,1000);

  let outer_div = document.createElement("div");
  let tbl = document.createElement("table");
  tbl.setAttribute("style","");
  let current_state = store.state;

  for(let i = 0; i != steps.length;++i){
    let tr = document.createElement("tr");
    let tdl = document.createElement("td");
    tdls.push(tdl);
    tdl.setAttribute("style",
     `border-right: 2px solid;border-right-color: rgba(100,100,100,150);`+
     `width: 100px;margin-top:10px;`
    );
    if (i <= current_state ) tdl.appendChild(get_time_info(i));
    let tdr = document.createElement("td");
    tdrs.push(tdr);
    if (i <= current_state) 
     tdr.setAttribute("style","text-align:left;padding:4px;");
    else 
     tdr.setAttribute("style","text-align:left;padding:4px;filter: opacity(0.4);");
    let title = document.createElement('h8');
    let title_str = document.createTextNode(steps[i].description);
    title.appendChild(title_str);
    tdr.appendChild(title);
    tr.appendChild(tdl);
    tr.appendChild(tdr);
    let tdl_spacer = document.createElement("td");
    tdl_spacer.setAttribute("style","width:20px;");
    tr.appendChild(tdl_spacer);
    tbl.appendChild(tr);
    let tr_spacer = document.createElement("tr");
    tr_spacer.appendChild(document.createElement("td"));
    tr_spacer.appendChild(document.createElement("td"));
    tr_spacer.setAttribute("style","height:15px;");
    tbl.appendChild(tr_spacer);
  }
  outer_div.appendChild(tbl);
  body_div.appendChild(outer_div);
  update_running_rollout_overview_dlg_store_info_timeline();
 }

 function build_running_rollout_overview_dlg_store_info(parent,status,cov,steps,store,tiles,index,rollout_id){
  function get_header_class() {
      if (status == undefined || status == undefined) return "bg-secondary";
      if (status == TILE_STATUS_OK) return "bg-primary";
      if (status == TILE_STATUS_WARN) return "bg-warning";
      if (status == TILE_STATUS_ERROR) return "bg-danger";
      return "bg-success";      
   };

   for(;parent.firstChild != null;) parent.removeChild(parent.firstChild);
   let outer_div = document.createElement("div");
   outer_div.setAttribute("style",`width:400px;`);

   let header_div = document.createElement("div");
   header_div.setAttribute("style",`position:relative;left:0px;top:0px;width:100%;`);
   header_div.setAttribute("class",get_header_class());
   let header_sub_div_top = document.createElement("div");
   header_sub_div_top.setAttribute("style","height:1em;");
   let header_sub_div_middle = document.createElement("div");
   header_sub_div_middle.setAttribute("style","color:#F8F8FF;padding-left:12px;");
   let header_sub_div_bottom = document.createElement("div");
   header_sub_div_bottom.setAttribute("style","color:#F8F8FF;padding-right:12px;margin-bottom:10px;text-align:right;");

   header_div.appendChild(header_sub_div_top);
   header_div.appendChild(header_sub_div_middle);
   header_div.appendChild(header_sub_div_bottom);
   let middle_text = document.createTextNode(store.title);
   let middle_h3 = document.createElement("h3");
   middle_h3.appendChild(middle_text);
   header_sub_div_middle.appendChild(middle_h3);
   let middle_main = middle_h3;
   let bottom_text = document.createTextNode(`${Math.trunc(cov*10000.0)/100}%`);
   let h5_elem = document.createElement("h5");
   h5_elem.appendChild(bottom_text);
   bottom_main = h5_elem;
   header_sub_div_bottom.appendChild(h5_elem);
   outer_div.appendChild(header_div);
   let body_div = document.createElement("div");
   body_div.setAttribute("style",`overflow:scroll;height:220px;`);
   build_running_rollout_overview_dlg_store_info_timeline(body_div,status,cov,steps,store,tiles,index);
   outer_div.appendChild(body_div);
   let footer_div = document.createElement("div");
   footer_div.innerHTML = `<div class="form-group" style="width:140px;margin:0 auto;">
    <button style="float:left;margin:8px;" 
                    id="XXXX" 
                    type="button" 
                    class="btn btn-outline-primary btn-sm"
                    onclick="kill_store('${store.title}','${rollout_id}')"
                    >Kill</button>
    <button style="float:right;margin:8px;" 
                    id="YYY" 
                    type="button" 
                    class="btn btn-outline-primary btn-sm"
                    onclick="restart_store('${store.title}','${rollout_id}')"
                    >Restart</button>                    
    </div>`;
   outer_div.appendChild(footer_div);
   parent.appendChild(outer_div);   
 }

 function build_running_rollout_overview_dlg (id,idx,data,header,body,footer){
  
   let cw = 300;
   let ch = 300;
   let radius = Math.floor(cw/2 * 0.45);
   let lw1 = 40;
   let attrs = extractAttributesFromStaccatoEntity(data.entity.raw);
  
   body.innerHTML =
   `
   <a href="?rollout=${attrs.id}" target="_blank">Open in Tab</a>
   <div style="text-align:center;align:center;" >
   <div>
    <div style="margin:10px;float:left;">
      <canvas id="running_rollout_overview_dlg_canvas" width="${cw}px" height="${ch}px" style="background-color:white;">
     </canvas>
    </div>
    <div id = "running_rollout_overview_dlg_store_info" style="width:auto;margin:auto;float:right;">
     <div style="width:100%;height:100%;text-align:center;color:rgba(140,140,140,150);">
     <h4 style="margin-top:150px;">Click on a market for full details.</h4></div>
    </div>
    </div>
    
    <div class="form-group" style="padding-left: 4px;padding-right: 4px;">
      <label for="rollout_overview_dlg_search_statemachines"></label>
      <input type="text" 
                  class="form-control" 
                  id="rollout_overview_dlg_input_search_statemachines" 
                  aria-describedby="search_statemachinesHelp" placeholder="Search Stores">
       <small id="rollout_overview_dlg_search_statemachinesHelp" class="form-text text-muted"></small>
    </div>

    <div style = "height: ${Math.floor(window.outerHeight*0.30)}px;overflow: scroll;">
     <div id="build_running_rollout_overview_dlg_details">

     </div>
    </div>
   `;
   console.log(attrs);
   let id2idx = new Map;

   let stores = attrs.stores;
   let steps = attrs.steps;

   for(let i = 0; i != stores.length;++i){
    id2idx.set(stores[i]["id"] , i);
   }

   let tiles = create_tiles_deck(
     document.getElementById("build_running_rollout_overview_dlg_details"), 
     {
       getSize : function () {return stores.length;},
       getTitle : function (idx) {return stores[idx]["name"];},
       getState : function(idx) {return 0;},
       getStatus: function(idx) { 
             return utilsTranslateHealthToTileStatus(stores[idx]["health"]);           
           },
       getCoverage: function(idx) { 
             return stores[idx]["coverage"];           
       },       
       getTile2StatesMapping  : function() { return {}; },
       getState2VisitedStates : function() { return {}; },
       getState2RootMapping   : function() { return {}; },
       getState2LabelMapping  : function() { 
         let m = {};
         for(let i = 0; i != steps.length; ++i) {
           m[i] = steps[i].description;
         } 
       },
       getStateLabels  : function() { 
         let v = [];
         for(let i = 0; i != steps.length; ++i) {
           v.push(steps[i].description);
         }
         return v;
       },
       getEnterTime : function (idx,state){
         return {secs_since_uptime:stores[idx]["entering_times"][state],msecs_since_uptime:0};         
       },
       getExitTime : function (idx,state) {
         return {secs_since_uptime:stores[idx]["exiting_times"][state],msecs_since_uptime:0};
       },
       getUpTime : function(){
         return data.attributes["start_time_unix_time"];
       },
       getEnterTimes          : function(idx) {return stores[idx]["entering_times"];},
       getExitTimes           : function(idx) {return stores[idx]["exiting_times"];}

     },
     {},
     {}
   );
   tiles.tiles_widget.register_click_tile_handler(
     function (index,data){
       build_running_rollout_overview_dlg_store_info(
         document.getElementById("running_rollout_overview_dlg_store_info"),
         tiles.tiles_widget.data.info.status[index],
         tiles.tiles_widget.data.info.cov[index],
         steps,
         data,
         tiles,
         index,
         attrs.id);
     }
   );


   let update_running_rollout_overview_func = function () {
       let attrs = extractAttributesFromStaccatoEntity(data.entity.raw);
       //Update Tiles
       stores = attrs.stores;
       for(let i = 0; i != stores.length;++i) {
        let tile_idx = id2idx.get(stores[i]["id"]);
        if (stores[i]["coverage"] != tiles.tiles_widget.data.info.cov[tile_idx]) 
         tiles.tiles_widget.set_coverage(tile_idx,stores[i]["coverage"]);
        let new_status = utilsTranslateHealthToTileStatus(stores[i]["health"]);
        if (new_status != tiles.tiles_widget.data.info.status[tile_idx])
         tiles.tiles_widget.set_status(tile_idx,new_status);
         tiles.tiles_widget.set_state(tile_idx,stores[i]["current_state"]);
       }       

       //console.log(attrs);
       let cov_critical = 0.0;
       let cov_fatal = 0.0;
       let cov_complete = 0.0;
       let n_critical = 0;
       let n_fatal = 0;
       let n_complete = 0;
       let n_stores = attrs.stores.length;

       for(let i = 0; i != n_stores; ++i){
         if (attrs.stores[i].health == "critical") { 
           cov_critical += 
            (1/n_stores)*attrs.coverage;
            ++n_critical;
          }
         else if (attrs.stores[i].health == "fatal") cov_fatal +=  
          (1/n_stores)*attrs.coverage;
         else if (attrs.stores[i].health == "complete") cov_complete +=  
          (1/n_stores)*attrs.coverage;        
       }



       let c = document.getElementById("running_rollout_overview_dlg_canvas");
       let ctx = c.getContext("2d");
       ctx.clearRect(0, 0, cw, ch);
       ctx.imageSmoothingEnabled = true;
       let center_x = cw/2;
       let center_y = ch/2;
       //background

       ctx.beginPath();

       ctx.shadowOffsetX = 2;
       ctx.shadowOffsetY = 2;
       ctx.shadowBlur = 4;
       ctx.shadowColor = 'rgba(0, 0, 0, 0.5)';
       ctx.lineWidth = lw1;
       ctx.strokeStyle = "rgba(173, 216, 230,0.2)";
       ctx.arc(center_x, center_y, radius, 0, 2 * Math.PI);
       ctx.stroke();

       //Coverage: ALL

       ctx.beginPath();
       ctx.lineWidth = lw1;
       ctx.strokeStyle = "rgba(0, 120, 204,0.8)";//"rgba(102, 204, 0,0.9)";
       ctx.arc(center_x, center_y, radius, - 1/2 * Math.PI, - 1/2 * Math.PI +  2 * Math.PI * attrs.coverage);
       ctx.stroke();

       ctx.font = "20px Arial";
       let t = `${ Math.round(attrs.coverage*10000)/100 }%`;
       let m = ctx.measureText(t);
       ctx.lineWidth = 1;
       ctx.fillStyle = ctx.strokeStyle;

       ctx.fillText(t, center_x - m.width/2, center_y+7);
       //ctx.fillText(t, center_x , center_y );

      //Coverage: COMPLETE
      let add_info = 0;

      if (cov_complete > 0.01) {
       ++add_info;
       ctx.beginPath();
       ctx.lineWidth = lw1;
       ctx.strokeStyle = "rgba(102, 204, 0,0.9)";
       ctx.arc(center_x, center_y, radius, - 1/2 * Math.PI, - 1/2 * Math.PI +  2 * Math.PI * (cov_complete+cov_critical+cov_fatal) );
       ctx.stroke();
       ctx.font = "10px Arial";
       let t = `${ Math.round(cov_complete*10000)/100 }%`;
       let m = ctx.measureText(t);
       ctx.lineWidth = 1;
       ctx.fillStyle = ctx.strokeStyle;

       ctx.shadowOffsetX = 0;
       ctx.shadowOffsetY = 0;
       ctx.shadowBlur = 0;

       ctx.font = "20px Arial";
       ctx.fillStyle = ctx.strokeStyle = "rgba(102, 204, 0,1.0)";

     
       ctx.fillText(t, 
        -m.width + center_x + 1.7*radius*Math.cos( 2 * Math.PI * (1/2*cov_complete +cov_critical+cov_fatal) - 1/2*Math.PI), 
        center_y + 1.7*radius*Math.sin( 2* Math.PI * (1/2*cov_complete +cov_critical+cov_fatal) - 1/2*Math.PI));

      }
      //Coverage: CRITICAL
      if (cov_critical > 0.01) {
        ++add_info;
       ctx.beginPath();
       ctx.lineWidth = lw1;
       ctx.strokeStyle = "rgba(255, 192, 24,0.8)";
       ctx.arc(center_x, center_y, radius, - 1/2 * Math.PI, - 1/2 * Math.PI +  2 * Math.PI * (cov_critical+cov_fatal) );
       ctx.stroke();

       ctx.font = "20px Arial";
       let t = `${ Math.round(cov_complete*10000)/100 }%`;
       let m = ctx.measureText(t);
       ctx.lineWidth = 1;
       ctx.fillStyle = ctx.strokeStyle = "rgba(255, 192, 24,1.0)";
       ctx.shadowOffsetX = 0;
       ctx.shadowOffsetY = 0;
       ctx.shadowBlur = 0;
       
       ctx.fillText(t, center_x + 1.4*radius*Math.cos( 2 * Math.PI * (cov_fatal+1/2*cov_critical) - 1/2*Math.PI), 
                       center_y + 1.4*radius*Math.sin( 2* Math.PI * (cov_fatal+1/2*cov_critical) - 1/2*Math.PI));

      }
      //Coverage: FATAL
      if (cov_fatal > 0.01) {
       ctx.beginPath();
       ctx.shadowOffsetX = 2;
       ctx.shadowOffsetY = 2;
       ctx.shadowBlur = 4;
       ctx.lineWidth = lw1;
       ctx.strokeStyle = "rgba(255, 50, 2,0.8)";
       ctx.arc(center_x, center_y, radius, - 1/2 * Math.PI, - 1/2 * Math.PI +  2 * Math.PI * (cov_fatal) );
       ctx.stroke();

       ctx.shadowOffsetX = 0;
       ctx.shadowOffsetY = 0;
       ctx.shadowBlur = 0;

       ctx.font = "20px Arial";
       let t = `${ Math.round(cov_fatal*10000)/100 }%`;
       ctx.fillStyle = ctx.strokeStyle = "rgba(255, 50, 2,1.0)";

     
       ctx.fillText(t, center_x + 1.4*radius*Math.cos( 2 * Math.PI * (1/2*cov_fatal) - 1/2*Math.PI), 
                       center_y + 1.4*radius*Math.sin( 2* Math.PI * (1/2*cov_fatal) - 1/2*Math.PI));

     }

    };

   update_running_rollout_overview_func();

   build_running_rollout_overview_dlg_update = setInterval(
     update_running_rollout_overview_func,1000);
 }

 function build_scheduled_rollout_overview_dlg(id,groupIndex,idx,data,header,body,footer){
   let scheduled_time = data.attributes.scheduled_time_unix_time;
   let jd = new Date(scheduled_time*1000);
   let now = Math.floor(Date.now()/1000);
   let due_string = "";
   if( scheduled_time - now > 0  ){
     let t = scheduled_time - now;
     let days = Math.floor(t / (3600*24));
     let hours = Math.floor((t - days * 3600*24) / 3600);
     let minutes = Math.floor((t - days * 3600*24 - hours * 3600) / 60);
     let seconds = Math.floor((t - days * 3600*24 - hours * 3600 - minutes * 60));
     if (days > 0)
      due_string = `${days} days`;
     if (hours > 0)
      due_string = due_string + ` ${hours} hours`;
     if (minutes > 0)
      due_string = due_string + ` ${minutes} minutes`;
     if (days == 0 && hours == 0)
     due_string = due_string + ` ${seconds} seconds`;
     due_string = `(due in ${due_string})`;
   }

   let launch_btn = "";
   if (data.attributes["processing_status"] == null || data.attributes["processing_status"] == "stopped"){
    launch_btn = `<button id="rollout_launch_btn" class="float-sm-left btn btn-primary mb-2">Launch</button>`;  
   }
   
   body.innerHTML = `
   <div style="" >
     <i class="material-icons" style="vertical-align:top;">schedule</i>
     <span style="vertical-align:top;">${jd.toLocaleString()}</span>
     <span>${due_string}</span>
     </div>        
   `;

   footer.innerHTML = `
   <table style="width:100%;">
    <tr>
      <td style=""></td>
    <td> <div style="margin-left: auto;margin-right: auto;width: 2em">${launch_btn}</div></td>
    <td style=""></td>
    </tr>
   </table>
   `;

   if (launch_btn.length > 0){
     let btn = document.getElementById("rollout_launch_btn");
     btn.onclick = function (){
      timeLineWidget.setActive(groupIndex,idx,false);
      $("#dlg_rollout_details").modal("hide");
      issue_rollaut_staccato_write_command (data);
     }
   }
 }

 function build_rollout_overview_dlg(id,groupIndex,idx,data){
   if (build_running_rollout_overview_dlg_update != null) {
     clearInterval(build_running_rollout_overview_dlg_update);
     build_running_rollout_overview_dlg_update = null;
   }
   let dlg = document.getElementById(id);
   let divs = [];
   for(let i = 0; i != dlg.firstElementChild.firstElementChild.children.length;++i){
     let tg = dlg.firstElementChild.firstElementChild.children[i];
     if(tg.tagName != "DIV") continue;
     divs.push(tg);
   } 
   let header = divs[0].firstElementChild;
   let body = divs[1];
   let footer = divs[2];
   for(;body.firstChild!=null;)body.removeChild(body.firstChild);
   for(;footer.firstChild!=null;)footer.removeChild(footer.firstChild);
   header.innerHTML = data.title;
   if (data.attributes["processing_status"] != null)
    build_running_rollout_overview_dlg(id,idx,data,header,body,footer);
   else
    build_scheduled_rollout_overview_dlg(id,groupIndex,idx,data,header,body,footer);
}
