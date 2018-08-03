let ws = require("ws");
let url1 = "ws://localhost:8182/";
let regular_get_installed = false;
let ws_channel1 = undefined; //new ws(url1);
let cmd = 
`RESTART_STATEMACHINES

This is P
This is S
`;
    

setInterval( () => {
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
            ws_channel1.send(cmd);
            setTimeout(f,5000); 
        };
        setTimeout(f,5000);
     }
    });
    ws_channel1.send(cmd); 
    });
    ws_channel1.on("close", () => {ws_channel1 = undefined;} ); 
}, 10 );


