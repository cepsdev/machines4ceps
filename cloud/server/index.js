const WebSocket = require('ws');
const os = require('os');

const host_name = os.hostname();

let sim_core_counter = 0;

let sim_cores = [
  
];


function Simcore(){
 this.url=undefined;
 this.signal_url=undefined;
 this.command_url=undefined;
 this.ws=undefined;
 this.name=undefined;
 this.uri=undefined;
 this.comm_layer = { frames : [] };
 this.index =undefined;
}

function Simcore(p){
 this.url=p.url;
 this.signal_url=p.signal_url;
 this.command_url=p.command_url;
 this.ws=p.ws;
 this.name=p.name;
 this.uri=p.uri;
 this.comm_layer = p.comm_layer;
 this.index =p.index;
}
Simcore.prototype.get_status = function () { return "N/A";}
Simcore.prototype.get_description = function () { return "N/A";}

sim_cores.push(new Simcore(  { 
      url:"ws://"+host_name+":8181",
      signal_url:"ws://"+host_name+":8182",
      command_url:"ws://"+host_name+":8192",
      ws:undefined,
      name:"Not Available",
      uri:"?",
      comm_layer : { frames : [] },
      index :  sim_core_counter
    }));

function get_sim_core_by_name(name){
 for(let e of sim_cores)
     if (e.name === name) return e;
 return undefined;
}

function get_sim_core_by_uri(uri){
 for(let e of sim_cores)
     if (e.uri === uri) return e;
 return undefined;
}

function sim_core_init(sim_core){

 let frame_names_received = (msg) => {
     sim_core.ws.removeListener('message', frame_names_received);
     sim_core.comm_layer.frames = JSON.parse(msg);
     sim_core.index = ++ sim_core_counter;
     console.log("Simulation Core '"+sim_core.name+"'@"+sim_core.uri+" online.");
     let frame_counter = 0;
     let sig_counter = 0;
     sim_core.signals = [];
     for(let frame of sim_core.comm_layer.frames){
         frame.index = frame_counter++;
         for(let sig of frame.signals){
             sig.index = sig_counter++;
             sim_core.signals.push(sig.name);             
         }
         //console.log(frame);
     }
    console.log(sim_core.ws._socket.remoteAddress);
 };
 let sim_uri_received = (msg) => {
    sim_core.uri = msg;
    sim_core.ws.removeListener('message', sim_uri_received);
    sim_core.ws.on("message",frame_names_received);
    sim_core.ws.send('comm_layer_frames',{});
 };
 
 let sim_name_received = (msg) =>  {
    sim_core.name = msg;
    sim_core.ws.removeListener('message', sim_name_received);
    sim_core.ws.on("message",sim_uri_received);
    sim_core.ws.send('sim_uri',{});
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
    res.render("index",{ page_title:"Home",
                         sim_cores : sim_cores,
                         sim_core : undefined});
});

app.get(/^\/(signaldetails__([0-9]+)__([0-9]+))|(\w*)$/, function(req, res) {
 if (req.params[3] != undefined) {
    let score = get_sim_core_by_uri(req.params[3]);
    if (score != undefined) {
         res.render("sim_main",{ page_title: score.name,
                                 sim_core : score}); 
    }
 } else {
   let score_idx = req.params[1];
   let sig_idx = req.params[2];
   let score = undefined;
   for(let s of sim_cores) if (s.index == score_idx){ score=s;break; }
   if (score === undefined) {res.status=404;res.send("404");return;} 
   let signalname = score.signals[sig_idx];
   if (signalname === undefined) {res.status=404;res.send("404");return;}
   let sig = undefined;
   for(let f of score.comm_layer.frames)
    for (let s of f.signals) if (s.index == sig_idx){sig = s;break;}
   if (sig === undefined) {res.status=404;res.send("404");return;}
   
   res.render("signal_details",{ page_title: score.name +"-"+ signalname,
                                 sim_core : score,
                                 signal:sig,
                                 signal_ws:score.signal_url});   
 }
});


http.createServer(app).listen(3000);