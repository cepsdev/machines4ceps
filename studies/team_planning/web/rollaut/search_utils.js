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


function rollaut_search_utils_connect_search_box(id, widget){
  $(`#${id}`).keyup(function(e){
    let t = $(`#${id}`).val();
    let vt = t.split(" ");
    let v = [];
    for(let i = 0; i < vt.length; ++i){
      if (vt[i].length == 0 || vt[i] == " " ) continue;
      if (vt[i].startsWith("\"")){
        let tt = vt[i].substring(1);
        if (tt.endsWith("\"")) 
         tt = tt.substring(0,tt.length-1);
        let s = tt;++i;
        for(;i<vt.length;++i){
          if (vt[i].length == 0 || vt[i] == " " ) continue;
          if (vt[i].endsWith("\"")){
            s += " "+ vt[i].substr(0,vt[i].length-1);
            break;
          }
          else s += " "+vt[i];
        }
        v.push(s);
        continue;
      }
      v.push(vt[i]);
    }
    if (v.length == 0){
      widget().applyFilter( function() {return true;});
    } else {
      widget().applyFilter(
        function(tile_idx,data){
          for(let i = 0; i != v.length;++i){
            if (data.title[tile_idx].indexOf(v[i]) != -1) return true;
          }
          return false;
        }
      );
    }
  });
}

