/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


/*

Part of the WebSocket API examples/tests.
 
Node.js client which connects to a running ceps instance on ws://localhost:8182.

*/



let ws = require("ws"); 

let url1 = "ws://localhost:8182/";
let ws_channel1 = undefined; 
let ws_channel2 = undefined; 

setInterval( () => {
        if (ws_channel1 != undefined) return;
        let ctr = 0;
        ws_channel1 = new ws(url1);
        ws_channel1.on("error", () => {} );
        ws_channel1.on("open", () => {
            console.log("Connected to "+ws_channel1.url);
            
            ws_channel1.send("WATCH\nsig1\nsig2");//WATCH\nvar_1\nvar_2\n...\nvar_n registers a monitoring handler for var_1 ... var_n 

            ws_channel1.on("message", (m)=>{
                ++ctr;
                console.log("update #",ctr,":",m);
                if(ctr % 10 == 0)  
                 try{ ws_channel1.send("EVENT\nEV"); } catch(e) {}
            });
        });
        ws_channel1.on("close", () => {ws_channel1 = undefined;} ); 
    }, 1000
);
 
setInterval( () => {
    if (ws_channel2 != undefined) return;
    let ctr = 0;
    ws_channel2 = new ws(url1);
    ws_channel2.on("error", () => {} );
    ws_channel2.on("open", () => {
        setInterval(
            () => {
                try{ ws_channel2.send("EVENT\nEV"); } catch(e) {}
            },
            100
        )
    });
    ws_channel2.on("close", () => {ws_channel1 = undefined;} ); 
}, 1000
);
