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

let RollAutStaccatoAPI = function (url,on_db_changed){
    THIS = {
      dbviews : [],
      main_socket : undefined,
      connected   : false,
      on_connect_handler : [],
      on_close_handler   : [],

      //
      //
      //  {
      //    loc: Location
      //    db : Database
      //    getTopLevelEntities:function (type,deleted)
      //  }
      //
      //
      create_db_view: function(loc){
        let r = {
          loc: loc,
          db : null,
          merge: function(diff){
            diff_merge(diff,THIS.db);
            return diff;
          },
          readAttr:function(attr,node){
            if (attr == null || node == null) return null;
            let v = node["children"];
            if (v == undefined ) return null;
            for(let i = 0; i != v.length;++i){
              if (v[i]["class"] != "attribute")
               continue;
              if (v[i]["name"] != attr) 
               continue;
              return v[i]["value"];
            }
            return null;
          },
          getTopLevelEntities:function (type,deleted){
            let r = [];
            for(let i = 0; i != this.db.length;++i){
              let v = this.db[i];  
              if (v["class"] == "entry" && v["deleted"] == deleted){
                for(let i = 0; i != v.children.length;++i){
                  let a = v.children[i];
                  if (a["class"] != "attribute" || a["name"] != "entity") continue;
                  if (a["value"] != type) continue;
                  r.push(v); break;
                }
              }
            }
            return r;
          }
        }
        THIS.dbviews.push(r);
        return r;
      },
      get_db_view: function (loc){
        for(let i = 0; i != THIS.dbviews.length;++i){
          if (THIS.dbviews[i]["loc"] == loc) return THIS.dbviews[i];
        }
        return null;
      },
      unregister_handler   : function (what,f){
        let v = undefined;
        if ("open" == what) v = THIS.on_connect_handler;
        else if ("close" == what) v = THIS.on_close_handler;
        for(let i = 0; i != v.length;++i) if (v[i] == f) {v[i]=undefined;break;}
      },
      register_handler   : function (what,f){
        let v = undefined;
        if ("open" == what) v = THIS.on_connect_handler;
        else if ("close" == what) v = THIS.on_close_handler;
        let slot = -1;
        if (v.length){
          for(let i = 0; i != v.length;++i) if (v[i] == undefined) {slot=i;break;}
        }
        if (slot != -1) v[slot] = f;
        else v.push(f);
      },
      pending_subscribers : { }, //topic -> list of handlers
      topic_subscribers   : { }, //channel -> list of handlers
      subscribe   : function(loc,force) {
        if (force == null) force = false;
        if (THIS.main_socket == null) return false;
        if (THIS.get_db_view(loc) != null && !force){
           return true;
        }
        if (THIS.main_socket.readyState != 1){
          let h = function(){
            THIS.main_socket.removeEventListener("open",h);
            THIS.main_socket.send(JSON.stringify({what:"command",name:"subscribe",params:[loc]}));
          };
          THIS.main_socket.addEventListener("open",h);
          return true;
        }         
        THIS.main_socket.send(JSON.stringify({what:"command",name:"subscribe",params:[loc]}));
        return true;
      },
      connect     : function(){
            THIS.connected = false;
            THIS.main_socket = new WebSocket("ws://"+url);
            THIS.main_socket.addEventListener("open", THIS.on_ws_initial_connect);
            THIS.main_socket.addEventListener("close", THIS.on_ws_close);
            THIS.main_socket.addEventListener("error", THIS.on_ws_error);
            THIS.main_socket.addEventListener("message",THIS.on_message);
      },
      on_ws_initial_connect: function (ev) {
            THIS.connected = true;
            for(let i = 0; i!=THIS.on_connect_handler.length;++i)
             if (THIS.on_connect_handler[i]!=undefined) THIS.on_connect_handler[i](THIS,THIS.on_connect_handler[i]);
      },
      on_ws_close: function (ev) {
            let connection_status = THIS.connected;
            THIS.connected = false;
            for(let i = 0; i!=THIS.on_close_handler.length;++i)
             if (THIS.on_close_handler[i]!=undefined) THIS.on_close_handler[i](THIS,connection_status,THIS.on_connect_handler[i]);
      },
      on_ws_error: function (ev) {
            
      },
      on_message: function (ev){
        let msg = JSON.parse(ev.data);
        
        if (msg.class == "diff"){
          let db_view = THIS.get_db_view(msg.watched_location);
          if (db_view == null){
            db_view = THIS.create_db_view(msg.watched_location);
          }
          if (db_view.db == null){
            db_view.db = msg.children[0].children;
            if (db_view.db == undefined) db_view.db = [];
            on_db_changed(db_view,null);
            return;
          } else {
            //MERGE
            if (msg.children.length == 0) return; //Nothing to do
            let changed_items = db_view.merge(msg.children[0].children);
            on_db_changed(db_view,changed_items);
          }
        }
      }
    };
    return THIS;
  }
