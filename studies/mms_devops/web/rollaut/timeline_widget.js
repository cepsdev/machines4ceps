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


let TimeLineWidget = function (parent, 
                               data, 
                               style_info) {
  let THIS = {
   groupedEntities : [],
   on_click : null,
   dom_cache : null,
   setActive(groupIndex,idx,b){
    THIS.groupedEntities[groupIndex].tiles.setActive(idx,b);
   },
   applyFilter(f){
     for(let i = 0; i != THIS.groupedEntities.length;++i ){
       if (THIS.groupedEntities[i].entities.length == 0) continue;
       THIS.groupedEntities[i].tiles.apply_filter(f);
     }
   },
   create   : function(){
    let entities_raw = data.getTopLevelEntities("rollout","false");
    let es = THIS.extractEntities(entities_raw);

    let t1 = Math.floor((Date.now() / 1000));
    let aDay = 24*3600;
    let today = Math.floor(t1 / (aDay)) * aDay +1;
    let tomorrow = today + aDay;
    let overmorrow = today + 2*aDay;
    let yesterday = today - aDay ;
    let ereyesterday = today - 2*aDay;

    let grouped_es = THIS.groupEntities(
      [
        {
          visible  : false,
          time     : ereyesterday,
          title    : (new Date(ereyesterday*1000)).toDateString() + " and older",
          entities : [],
        },
        {
          visible  : true,
          time     : yesterday,
          title    : "Yesterday <small>("+(new Date(yesterday*1000)).toDateString()+")</small>",
          entities : [],
        },
        {
          visible  : true,
          time     : today,
          title    : "Today <small>("+(new Date(today*1000)).toDateString()+")</small>",
          entities : []
        },
        {
          visible  : true,
          time     : tomorrow,
          title    : "Tomorrow <small>("+(new Date(tomorrow*1000)).toDateString()+")</small>",
          entities : []
        },
        {
          visible  : false,
          time     : overmorrow,
          title    : (new Date(overmorrow*1000)).toDateString() + " and younger",
          entities : []
        }          
      ],
      es
      );
    THIS.groupedEntities = grouped_es;
   },
   extractAttributes: function (e){
     let r = {
       coverage : null,
       health: null,
       title : e["name"]
     }
     if (e.children == null) return r;
     for(let i = 0; i != e.children.length;++i){
        if (e.children[i]["class"] != "attribute") 
          continue;
        r[e.children[i]["name"]] = e.children[i]["value"];

      }
     return r;
   },
   translateHealthToTileStatus: function (health){
      if (health == null) return TILE_STATUS_INACTIVE;
      return { 
            "ok":TILE_STATUS_OK,
            "complete":TILE_STATUS_DONE,
            "failed":TILE_STATUS_ERROR,
            "failure":TILE_STATUS_ERROR,
            "fatal":TILE_STATUS_ERROR,
            "critical":TILE_STATUS_WARN,
            "n/a":TILE_STATUS_INACTIVE
        } [health];       
   },

   mergeAttributes:function(e_new,e_orig){
     if (e_new.children == null) return;
     for(let i = 0;i != e_new.children.length;++i){
       let a = e_new.children[i];       
       if (a["class"] == null || a["class"] != "attribute") continue;
       let found = false;
       for(let j = 0; j != e_orig.children.length;++j){
         let ao = e_orig.children[j];
         if (ao["id"] == null) continue;
         if (ao["id"] != a["id"]) continue;
         if (a["deleted"] == "true") ao["value"] = null;
         else ao["value"] = a["value"];
         found = true; break;
       } 
       if (!found && a["deleted"] != "true"){
         e_orig.children.push(a);
       }
     }
   },

   updateEntity : function (groupIdx, entityIdx, new_data) {
      e_orig = THIS.groupedEntities[groupIdx]
                   .entities[entityIdx];
      THIS.mergeAttributes(new_data,e_orig.raw);
      let tiles = THIS.groupedEntities[groupIdx]["tiles"];
      let attributes = THIS.extractAttributes(new_data);
      if (attributes.coverage != null)
       tiles.tiles_widget.set_coverage(entityIdx,attributes.coverage);
      if (attributes.health != null)
       tiles.tiles_widget.set_status(
         entityIdx,
         THIS.translateHealthToTileStatus(attributes.health)
       );
      if (attributes.title != tiles.tiles_widget.data.info.title[entityIdx])
        tiles.tiles_widget.set_title(entityIdx,new_data["name"]);
       tiles.tiles_widget.data.up_since = attributes.start_time_unix_time;
      if (attributes.processing_status != null)
        THIS.setActive(groupIdx,entityIdx,true);
      if(attributes.scheduled_time_unix_time != null)
       e_orig.time = attributes.scheduled_time_unix_time;
   },
   deleteEntity: function(groupIndex,Index){
    THIS.groupedEntities[groupIndex].entities.splice(Index,1);
    THIS.groupedEntities[groupIndex]["tiles"].remove_entity(Index);
   },
   computeGroupIndex : function(groups,e){
     let e_time = e.time;
     for(let grp = 0;grp + 1 < groups.length;++grp){
       if (e_time < groups[grp+1].time) return grp;
     }
     return groups.length - 1;
   },
   update : function(changed_entities){
     let g = 0;
     let j = 0;
     let processed_ids = new Map;
     for(let i = 0; i != changed_entities.length;++i){
       let e = changed_entities[i];
       let e_orig = null;
       let tiles = null;
       let found = false;
       let deleted = false;
       for(g = 0; g != THIS.groupedEntities.length;++g)
       {
        for(j = 0; j != THIS.groupedEntities[g].entities.length;++j){
          if(THIS.groupedEntities[g].entities[j].id != e.id) continue;
          if (processed_ids[e.id]) continue;
          if (e.deleted != "true"){
           THIS.updateEntity(g,j,e);
           let grp_idx_after_update = 
            THIS.computeGroupIndex(THIS.groupedEntities,THIS.groupedEntities[g].entities[j]);
           if (grp_idx_after_update != g){
            processed_ids[e.id] = true;
             let entity = THIS.groupedEntities[g].entities[j];
             let state = THIS.groupedEntities[g].tiles.tiles_widget.data.info.state[j];
             let status = THIS.groupedEntities[g].tiles.tiles_widget.data.info.status[j];
             let cov = THIS.groupedEntities[g].tiles.tiles_widget.data.info.cov[j];
             let title = THIS.groupedEntities[g].tiles.tiles_widget.data.info.title[j];
             let visibility = THIS.groupedEntities[g].tiles.tiles_widget.data.visibility[j];
             THIS.deleteEntity(g,j);
             let indices = THIS.insertIntoGroupedEntities(THIS.groupedEntities,entity);
             let elem_data = {
             state : 0,
              status: status,
              title : title,
              cov   : cov
             }
             THIS.groupedEntities[indices.groupIndex]["tiles"].insert_entity(indices.idx,elem_data);             
            }
          }
          else deleted = true;
          found = true;
          break;
        }
        if (!found) continue;
        break;
       }
       if (found && deleted){
         THIS.deleteEntity(g,j);
       } else if (!found && e["deleted"]!="true" && processed_ids[e.id] == null){
         //CASE: New Entity
         let v = THIS.extractEntities([e]);
         if (v.length == 0) continue;
         e = v[0];
         let indices = THIS.insertIntoGroupedEntities(THIS.groupedEntities,e);
         let attrs = THIS.extractAttributes(e.raw);
         let tile_status = THIS.translateHealthToTileStatus(attrs.health);
         let elem_data = {
             state : 0,
             status:tile_status,
             title : attrs.title,
             cov   : attrs.coverage
         }
         THIS.groupedEntities[indices.groupIndex]["tiles"].insert_entity(indices.idx,elem_data);
       }
     }
   },
   insertIntoGroupedEntities : function (eg,e){
     let curGroup = 0;
     for(;curGroup + 1 < eg.length;++curGroup){
       if (eg[curGroup+1].time > e.time) break;
     } 
     let idx = 0;
     for(;idx < eg[curGroup].entities.length;++idx)
     {
       if (eg[curGroup].entities[idx].time > e.time) break;
     }
     if (idx == eg[curGroup].entities.length ) eg[curGroup].entities.push(e);
     else eg[curGroup].entities.splice(idx,0,e);
   
     return{
       groupIndex:curGroup,
       idx:idx
     };
   },
   buildDom : function(){
     THIS.dom_cache = [];
     let content = document.createElement("div");
     for(let i = 0; i != THIS.groupedEntities.length;++i){
       let dom_rep;
       THIS.dom_cache.push(
        dom_rep = {
           checkbox : null,
           body     : null
         }
       );
       let current_grp_index = i;
       let g = THIS.groupedEntities[i];
       let header = document.createElement("div");
       let body = document.createElement("div");
       header.setAttribute("style",
       `border-bottom: 2px solid;margin:4px;padding:2px;`);
       let hh = document.createElement("div");
       
       hh.innerHTML = `<h4>${g.title}</h4>`;//Child(title);
       
       let checkbox = dom_rep.check_box = document.createElement("div");
       checkbox.setAttribute(
         "style",
         `float:left;margin-top:5px;
         `
       );
       checkbox.setAttribute("class","rollaut_timeline_checkbox");
       if (g.visible){
         checkbox.innerHTML = `<i class="material-icons md-30">check_box</i>`;
       }
       else {
         body.setAttribute("style","display:none;");
         checkbox.innerHTML = `<i class="material-icons md-18">check_box_outline_blank</i>`;
       }


       header.appendChild(checkbox);
       header.appendChild(hh);
       content.appendChild(header);
       content.appendChild(body);
       let raw_data = [];
       for(let j = 0; j != g.entities.length;++j){
         raw_data.push(g.entities[j].raw);
       }
       dom_rep.body = body;

       g["tiles"] = create_tiles_deck(body, 
         /*Data Model*/{
           getSize : function () {return raw_data.length;},
           getTitle : function (idx) {return raw_data[idx]["name"];},
           getState : function(idx) {return 0;},
           getStatus: function(idx) { 
             return THIS.translateHealthToTileStatus(THIS.extractAttributes(raw_data[idx]).health);           
           },
           getCoverage: function(idx) { 
             return THIS.extractAttributes(raw_data[idx]).coverage;           
           },
           getTile2StatesMapping  : function() { return {}; },
           getState2VisitedStates : function() { return {}; },
           getState2RootMapping   : function() { return {}; },
           getState2LabelMapping  : function() { return {}; },
           getStateLabels         : function() { return []; },
           getEnterTime           : function(idx,state) { return 0; },
           getExitTime            : function(idx,state) { return 0; },
           getUpTime              : function() {return null;},
           getEnterTimes          : function(idx) {return [];},
           getExitTimes           : function(idx) {return [];}
        }
       );
       g["tiles"]["tiles_widget"].register_click_tile_handler(
         function (index,data){
          if (THIS.on_click != null){
           data["entity"] = THIS.groupedEntities[i].entities[index];
           data["attributes"] = THIS.extractAttributes(THIS.groupedEntities[i].entities[index].raw);
           THIS.on_click(index,data,current_grp_index);
          }
         }
       );
       checkbox.addEventListener("click",function (ev){
         g.visible = !g.visible;
         if (g.visible){
          body.setAttribute("style","display:block;");
          checkbox.innerHTML = `<i class="material-icons md-30">check_box</i>`;
         } else {
          body.setAttribute("style","display:none;");
          checkbox.innerHTML = `<i class="material-icons md-18">check_box_outline_blank</i>`;
        }                 
       });
     }
     parent.appendChild(content);
   },
   extractEntities: function (rawData){
     let r = [];
     for (let i = 0; i != rawData.length;++i){
       r.push(
         {
           raw: rawData[i],
           id: parseInt(rawData[i]["id"],10),
           time: data.readAttr("scheduled_time_unix_time", rawData[i])
         }
       );       
     }     
     return r.sort(function (lhs,rhs){ return lhs.time - rhs.time; });
   },
   groupEntities: function(groups,sorted_entities){
     let cur_group = 0;
     let ng = groups.length;
     for(let i = 0; i < sorted_entities.length;++i){
       let e = sorted_entities[i];
       if (cur_group + 1 < ng && e.time >= groups[cur_group+1].time)
       {
         for(;cur_group < ng;++cur_group){
          if( e.time >= groups[cur_group].time)
          {
            if (cur_group+1 == ng) break;
            if (groups[cur_group+1].time > e.time ) break;
          } else break;
         }
       }
       groups[cur_group].entities.push(e);
     }
     return groups;
   }
 };
 THIS.create();
 THIS.buildDom();
 return THIS;
}
