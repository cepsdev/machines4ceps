const WebSocket = require('ws');

let sim_cores = [
    { url:"ws://localhost:8181/",
      ws:undefined,
      name:"Not Available", 
      comm_layer : { frames : [] }
    }
];

function get_sim_core_by_name(name){
 for(let e of sim_cores)
     if (e.name === name) return e;
 return undefined;
}

function sim_core_init(sim_core){

 let frame_names_received = (msg) => {
     sim_core.ws.removeListener('message', frame_names_received);
     sim_core.comm_layer.frames = JSON.parse(msg);
     console.log("Simulation Core '"+sim_core.name+"' online.");
 };
 let sim_name_received = (msg) =>  {sim_core.name = msg;
    sim_core.ws.removeListener('message', sim_name_received);
    sim_core.ws.on("message",frame_names_received);
    sim_core.ws.send('comm_layer_frames',{});
 };

 sim_core.ws.on("message",sim_name_received);
 sim_core.ws.send('sim_name',{});
}


function check_remote_sim_cores() {
    for(let core_info of sim_cores){
     if (core_info.ws === undefined){
         core_info.ws = new WebSocket(core_info.url);
         core_info.ws.on("error", () => {core_info.ws=undefined;} );
         core_info.ws.on("open", () => {sim_core_init(core_info);} );
         core_info.ws.on("close", () => {core_info.ws=undefined;} );
     }
    }
}

check_remote_sim_cores();
setInterval(check_remote_sim_cores,500);


let express = require("express");
let path = require("path");
let http = require("http");
let app = express();
let publicPath = path.resolve(__dirname, "public");

app.set("views", path.resolve(__dirname, "views"));
app.set("view engine", "ejs");

app.use(express.static(publicPath));

app.get("/", function(req, res) {
 console.log(sim_cores);
 res.render("index",{sim_cores : sim_cores});
});

http.createServer(app).listen(3000);