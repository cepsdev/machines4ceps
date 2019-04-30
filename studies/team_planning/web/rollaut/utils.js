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

function diff_merge(db,diff){
  
}


function extractAttributesFromStaccatoEntity(e){
 
    let r = {
    }
    if (e.id != undefined) r["id"] = e.id; 
    for(let i = 0; i != e.children.length;++i){
       if (e.children[i]["class"] != "attribute") 
         continue;
       r[e.children[i]["name"]] = e.children[i]["value"];
     }
    return r;
}

function utilsTranslateHealthToTileStatus(health){
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
}


function issue_rollaut_staccato_write_command (data){
 staccatoAPI.main_socket.send(JSON.stringify(
   { what  : "command",
     name  : "write_attribute_value",
     params: [
       { loc       : "rollouts/scheduled/",
         entity_id : data.entity.raw.id.toString(),
         attribute : "scheduled_time_unix_time",
         value     : Math.round(Date.now()/1000) 
       }
     ]
   }
 ));
}

function staccato_append_attribute (loc,name,attr,v){
 staccatoAPI.main_socket.send(JSON.stringify(
   { what  : "command",
     name  : "append_attribute",
     params: [
       { loc         : loc,
         entity_name : name,
         attribute   : attr,
         value       : v 
       }
     ]
   }
 ));
}
