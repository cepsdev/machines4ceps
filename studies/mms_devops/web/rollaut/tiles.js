const TILE_STATUS_OK    = 0;
const TILE_STATUS_DONE  = 1;
const TILE_STATUS_ERROR = 2;
const TILE_STATUS_WARN  = 3;

let ceps_tiles_component = function (parent, data) {
 let THIS = {
    tile_width               : 200,
    tile_height_em           : 3.0,
    progress_bar_height_em   : 0.5,
    dom_cache                : [],
    tileidx2dom_slot         : [],
    data                     : data,
    mm_handler               : undefined,
    register_move_over_tile_handler : function (f) {THIS.mm_handler = f;},
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

    set_coverage(tile_idx,cov){
     THIS.data.info.cov[tile_idx] = cov;
     
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
 return THIS; 
};
