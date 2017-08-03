let ws = require("ws");
let url1 = "ws://localhost:8182/";
let url2 = "ws://localhost:8182/";
let url3 = "ws://localhost:8182/";

let ws_channel1 = undefined; //new ws(url1);
let ws_channel2 = undefined; //new ws(url1);
let ws_channel3 = undefined;

let start_ws1_client = false;
let start_ws2_client = true;
let start_ws3_client = true;

if (start_ws1_client) setInterval( () => {
    if (ws_channel1 != undefined) return;
    ws_channel1 = new ws(url1);
    regular_get_installed = false;
    ws_channel1.on("error", () => {} );
    ws_channel1.on("open", () => {
    console.log("Connected to "+ws_channel1.url);
    ws_channel1.on("message", (m)=>{
     console.log(m);
     if (!regular_get_installed){
        regular_get_installed = true;
        let f = ()=>{
            if (ws_channel1 === undefined) return;
            ws_channel1.send("GET_UPDATE");
            setTimeout(f,50); 
        };
        setTimeout(f,100);
     }
    });
    ws_channel1.send("WATCH\nsig1\nsig2"); 
    });
    ws_channel1.on("close", () => {ws_channel1 = undefined;} ); 
}, 10 );

if (start_ws2_client) setInterval( () => {
    if (ws_channel2 != undefined) return;
    ws_channel2 = new ws(url2);
    ws_channel2.on("error", () => {} );
    ws_channel2.on("open", () => {
     ws_channel2.on("message", (m)=>{
      let msg = JSON.parse(m);
      //console.log(msg);
      if(!msg.ok){ ws_channel2.close(); ws_channel2 = undefined;}
      if (msg.signals != undefined) console.log(msg.signals);
     });
    ws_channel2.send("WATCH\nsig1\nsig2");
    ws_channel2.send("GET_UPDATE\n100"); 
    });
    ws_channel2.on("close", () => {ws_channel2 = undefined;} ); 
 }, 250 );


if (start_ws3_client) setInterval( () => {
    if (ws_channel3 != undefined) return;
    let ready = false;
    ws_channel3 = new ws(url3);
    ws_channel3.on("error", () => {} );
    ws_channel3.on("open", () => {
     ready = true;
     ws_channel3.on("message", (m)=>{
      let msg = JSON.parse(m);
      //console.log(msg);
      if(!msg.ok){ ready=false;ws_channel3.close();ws_channel3=undefined;console.log(msg);}
      if (msg.reason != undefined) console.log(msg.reason);
     });

     let f = () => { 
         let err_occured = false;
         if(ws_channel3 == undefined || !ready) return; 
         if(ws_channel3) try{ ws_channel3.send("SET_VALUE\nsig1\nnumber\n0.2"); } catch(e) {err_occured = true;} if (!err_occured)setTimeout(f,1500);}
     setTimeout(f,1500);
    
    let g = () => { 
         let err_occured = false;
         if(ws_channel3 == undefined || !ready) return; 
         if(ws_channel3) try{ ws_channel3.send("EVENT\nEV"); } catch(e) {err_occured = true;} if (!err_occured)setTimeout(g,500);}
     setTimeout(g,500);
    });
    

    ws_channel3.on("close", () => {ws_channel3 = undefined;} ); 
 }, 250 );
