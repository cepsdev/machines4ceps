

//Module requirements
const WebSocket = require('ws');
const os = require('os');
const { spawn,execFile} = require('child_process');
const fs = require('fs');
const path = require('path');
const express = require("express");
const http = require("http");
const chalk = require('chalk');
const dns = require("dns");
const fs_mv = require("mv");







const host_name = os.hostname();
const ceps_executable = "./ceps";
const ceps_default_args = ["--quiet"];
const ceps_prelude = "../.ceps/prelude.ceps";
const ceps_publisher = "../.ceps/publisher.ceps";
const sim_nodes_root = "../sim_nodes";
const sim_nodes_trash = "../trash";

const dbc_importer = "import_dbc.ceps";
const dbc_lexer = "dbc.ceps.lex";
let publish_port_start = 10100;
let publish_port_delta = 20;

let sim_core_counter = 0;
let sim_cores = [
  
];

let  sim_srcs = []; 

const command_port = 9191;
const command_ws_url = "ws://"+host_name+":"+command_port.toString();


/*MASTER HUB ===>*/
const master_hub_host = "localhost";
const master_hub_port = 8181;
const master_hub_port_wsapi = 8182;
const master_hub_process_check_interval_ms = 100;
const master_hub = "hub.ceps";
let master_hub_process = undefined;
let streaming_endpoints = [];

function get_streaming_endpoint_by_ip(ipaddr){
    let r = /^::ffff:([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)$/;
    match = r.exec(ipaddr);
    let to_match = undefined;
    if (match != undefined){
        to_match = match[1];
    } else {
        r = /^([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)$/; 
        match = r.exec(ipaddr);
        if (match != undefined){
            to_match = match[1];
        }
    }
    if (to_match == undefined) return undefined;
    for(e of streaming_endpoints) {
        if (e.host.valueOf() == to_match.valueOf()) return e;
    }
    return undefined;
}

setInterval(()=>{
    if (master_hub_process != undefined) return;
    let params = [ceps_prelude, master_hub, "--vcan_api", master_hub_port,"--ws_api", master_hub_port_wsapi, ceps_default_args ];
    master_hub_process = spawn(`${ceps_executable}`,params);
    master_hub_process.stdout.on('data', (data) => {
       console.log(chalk.yellow(`${data}`));
    });
    master_hub_process.stderr.on('data', (data) => {
       console.log(chalk.redBright(`${data}`));
    });
    master_hub_process.on('close', (code) => {
       master_hub_process = undefined;
       console.log(chalk.red(`Master Hub exited with code ${code}`));
    });

},master_hub_process_check_interval_ms);


let ws_api_master_hub = undefined;//new WebSocket.Server({port: command_port});
let ws_api_master_hub_open = false;
setInterval(()=>{
 if (ws_api_master_hub != undefined) return;
 ws_api_master_hub_open = false;
 ws_api_master_hub = new WebSocket("ws://"+master_hub_host+":"+master_hub_port_wsapi);
 ws_api_master_hub.on("error", () => {ws_api_master_hub=undefined;console.log(chalk.red(`***Error: Master Hub WS_API `));} );
 ws_api_master_hub.on("open", () => { 
     ws_api_master_hub_open = true;
     let ws_endpoint = ws_api_master_hub;
     let ws_endpoint_on_msg_set = false;
     let f = () => {
         if (!ws_endpoint_on_msg_set) ws_endpoint.on("message",(msg)=>{
             try{
              let m = JSON.parse(msg);
              if (m.ok){
                streaming_endpoints = [];
                for(e of m.endpoints){
                    if (/^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/.test(e.host))
                     streaming_endpoints.push(e);
                    else dns.resolve4(e.host, (err,a) =>{
                        if (err != undefined) return;
                        streaming_endpoints.push({host:a[0],port:e.port});
                    } );
                }
              }
             } catch (err){
                console.log(err);                
             }
         });
         ws_endpoint_on_msg_set = true;
         ws_endpoint.send("GET_KNOWN_STREAMING_ENDPOINTS");         
         if (ws_endpoint == ws_api_master_hub) setTimeout(f,250);
     }; 
     setTimeout(
         f , 250
     );
 } );
 ws_api_master_hub.on("close", () => {ws_api_master_hub=undefined;} );
},master_hub_process_check_interval_ms);

/*<== MASTER HUB*/


function Sim_source() {
    this.path;
    this.pkg_info = {name:undefined,uri:undefined,author:undefined,modules:[]};
    this.main_port = undefined;
    this.pkgfile_content = undefined;
    this.name = undefined;
    this.views = [];
}
function Sim_source(name) {
    this.path;
    this.pkg_info = {name:name,uri:undefined,author:undefined,modules:[]};
    this.main_port = undefined;
    this.pkgfile_content = undefined;
    this.name = name;
    this.views = [];
}
function Sim_source(name,pkg_info) {
    this.path;
    this.pkg_info = pkg_info;
    this.main_port = undefined;
    this.pkgfile_content = undefined;
    this.name = name;
    this.views = [];
}

function instantiate_sim_info(srcs,cores){
 srcs.forEach( (src) => {
    let core = new Simcore(
     {
      url:"ws://"+host_name+":"+src.pkg_info.base_port.toString(),
      signal_url:"ws://"+host_name+":"+(src.pkg_info.base_port+3).toString(),
      command_url:"ws://"+host_name+":"+(src.pkg_info.base_port+10).toString(),
      ws:undefined,
      name:src.name,
      uri:src.pkg_info.uri,
      comm_layer : { frames : [] },
      index :  sim_core_counter++,
      src:src,
      process : undefined,
      process_launching : false,
      status : "down",
      exit_code : "unknown"
    });
    cores.push(core);
 });
}


function get_index_of_sim_src_by_name(srcs,name){
    for(let i = 0; i != srcs.length; ++i){
        if (srcs[i].name == name) return i;
    }
    return -1;
}

function get_index_of_sim_src_by_uri(srcs,name){
    for(let i = 0; i != srcs.length; ++i){
        if (srcs[i].pkg_info === undefined) continue;
        if (srcs[i].pkg_info.uri === name) return i;
    }
    return -1;
}

function uniquify_uri(srcs,name){
    let j = 0;
    let t = name;
    while(-1 != get_index_of_sim_src_by_uri(srcs,t) ){
        ++j;
        t = name + "_" + j.toString(); 
    }
    return t;
}

function uniquify_name(srcs,name){
    let j = 0;
    let t = name;
    while(-1 != get_index_of_sim_src_by_name(srcs,t) ){
        ++j;
        t = name + " (" + j.toString()+")"; 
    }
    return t;
}

function make_sim_src_info_given_sim_dir(directory,
                                         subd,
                                         uniquify_name_clbk,
                                         uniquify_uri_clbk ){
 let files = fs.readdirSync(path.join(directory,subd));
 let sim_src = undefined;
 files.forEach( (fname) => {
      if (fname == "package.json"){
          let pkg_json_path = path.join(directory,subd,"package.json");
          let pkg_ceps_path = path.join(directory,subd,"package.ceps");
          
          let content = fs.readFileSync(pkg_json_path, 'utf8');
          let jsn = JSON.parse(content);
          let modified = false;
          if (! ("name" in jsn) ){
              console.warn("***Warning. Skipping '"+pkg_json_path+"': no name field.");
              return;
          }
          let orig_name = jsn.name;
          jsn.name = uniquify_name_clbk(jsn.name);
          modified = jsn.name != orig_name; 
          if (! ("modules" in jsn) || jsn.modules.length === undefined || jsn.modules.length === 0 ){
              console.warn("***Warning. Skipping '"+pkg_json_path+"': 'modules' field missing or illformed.");
              return;
          }
          if (!("uri" in jsn)){
              modified = true;
              let s = jsn.name;
              s = uniquify_uri_clbk(s.replace(/\s/g,"_").replace(/\(/g,"").replace(/\)/g,"").replace(/\//g,"").replace(/\./g,"").replace(/:/g,"") );
              console.warn("***Warning. No 'uri' field in '"+pkg_json_path+"': a default value was generated '"+s+"'.");
              jsn.uri = s;
          }
          jsn.base_port = publish_port_start;
          publish_port_start += publish_port_delta;
         
          sim_src = new Sim_source(jsn.name,jsn);
         
          {
           let d = new Date(Date.now());
           let fromatted = d.toLocaleString();
           fs.writeFileSync(pkg_ceps_path,
`/* Generated by cepscloud@Node.js ${fromatted} */
package {
 name { "${sim_src.name}"; };
 uri { "${sim_src.pkg_info.uri}";};
 vcan_api_port {"${sim_src.pkg_info.base_port+2}";};
 ws_api_port {"${sim_src.pkg_info.base_port+3}";};
 vcan_api_host_name {"${host_name}";};
 directory_master{ host_name{"${host_name}";};port{"${master_hub_port}";}; };
};
val publisher_baseport = "${sim_src.pkg_info.base_port}";
val publisher_cmd_port = "${sim_src.pkg_info.base_port+10}";
val publisher_signal_port = "${sim_src.pkg_info.base_port+1}";
`          ,           
           'utf8');
          }
          sim_src.path = subd;
      }
  } ); 
 return sim_src;
}

function walk_dir_and_fetch_sim_src_infos(directory){
 let subds = fs.readdirSync(directory);
 let r = [];
 if (subds.length == 0) return r;
 subds.forEach( (subd) => {
  let sim_src = make_sim_src_info_given_sim_dir(directory,subd,(s)=>{return uniquify_name(r,s);},(s)=>{return uniquify_uri(r,s);});
  if (sim_src != undefined) r.push(sim_src);
 });
 return r;
}

function Simcore(){
 this.url=undefined;
 this.signal_url=undefined;
 this.command_url=undefined;
 this.ws=undefined;
 this.name=undefined;
 this.uri=undefined;
 this.comm_layer = { frames : [] };
 this.index =undefined;
 this.src = undefined;
 this.process = undefined;
 this.process_launching = false;
 this.dont_launch = false;
 this.status = "down";
 this.exit_code = "unknown";
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
 this.src = p.src;
 this.process = p.process;
 this.process_launching = false;
 this.dont_launch = false;
 this.status = p.status;
 this.exit_code = p.exit_code;
}

Simcore.prototype.get_status = function () { return this.status;}
Simcore.prototype.get_description = function () { return "N/A";}
Simcore.prototype.get_view_path = function (view_name){
 for(let i = 0; i != this.src.views.length;++i )
     if (this.src.views[i] == view_name)
        return path.join(sim_nodes_root,this.src.path,"views",view_name+".ejs");
 return undefined;
}

/*sim_cores.push(new Simcore(  { 
      url:"ws://"+host_name+":8181",
      signal_url:"ws://"+host_name+":8182",
      command_url:"ws://"+host_name+":8192",
      ws:undefined,
      name:"Not Available",
      uri:"?",
      comm_layer : { frames : [] },
      index :  sim_core_counter
    }));
*/

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
     //sim_core.comm_layer.frames = JSON.parse(msg);
     sim_core.index = ++ sim_core_counter;
     console.log("Simulation Core '"+sim_core.name+"'@"+sim_core.uri+" online.");
     sim_core.status = "running";
     //let frame_counter = 0;
     //let sig_counter = 0;
     sim_core.signals = [];
     /*for(let frame of sim_core.comm_layer.frames){
         frame.index = frame_counter++;
         for(let sig of frame.signals){
             sig.index = sig_counter++;
             sim_core.signals.push(sig.name);             
         }
         //console.log(frame);
     }*/
    //console.log(sim_core.ws._socket.remoteAddress);
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

function make_call_sequence(core_info, options){
 let call_sequence = [];
 call_sequence.push(`${ceps_prelude}`);
 call_sequence.push(`${dbc_lexer}`);

 if (options == undefined || options['package_info'] == undefined || options['package_info'])
    call_sequence.push(path.join(`${sim_nodes_root}`,`${core_info.src.path}`,"package.ceps"));

 core_info.src.pkg_info.modules.forEach( (f) => {call_sequence.push(path.join(`${sim_nodes_root}`,`${core_info.src.path}`,`${f}`)); });
 call_sequence.push(`${dbc_importer}`);
 if (options == undefined || options['start_publisher'] == undefined || options['start_publisher']) call_sequence.push(`${ceps_publisher}`);         
 if (options == undefined || options['start_ws_api'] == undefined || options['start_ws_api']) {
     call_sequence.push("--ws_api");     
     call_sequence.push(`${core_info.src.pkg_info.base_port+3}`);
 }
 call_sequence = call_sequence.concat(ceps_default_args);
 return call_sequence;
}

function spawn_and_trace_ceps_process(info,call_sequence){
    let p = spawn(`${ceps_executable}`,call_sequence);
    p.stdout.on('data', (data) => {
       console.log(chalk.yellow(`${data}`));
    });
    p.stderr.on('data', (data) => {
       console.log(chalk.redBright(`${data}`));
    });
    p.on('close', (code) => {
        if (code!=0) console.log(chalk.red(`[sim core terminated][pid: ${p.pid}][exit code: ${code}] "${info}"`));
        else console.log(chalk.yellow(`[sim core terminated][pid: ${p.pid}][exit code: ${code}] "${info}"`));
    });
    return p;
}

function check_remote_sim_cores() {
    for(let core_info of sim_cores){
     if (core_info.dont_launch){
         if (core_info.ws) core_info.ws.close();
         core_info.ws = undefined;
         continue;
     }
     if (core_info.process === undefined){
         core_info.comm_layer = { frames : [] };
         core_info.process_launching = true;
         core_info.status = "launching";          
         if (core_info.ws) {core_info.ws.close();}
         core_info.ws = undefined;
         
         let call_sequence = make_call_sequence(core_info); 
         
         let p = spawn(`${ceps_executable}`,call_sequence);
         console.log(chalk.bold(`[spawned sim core][pid: ${p.pid}]${ceps_executable} `,call_sequence.join(" "),"\n"));
         
         p.stdout.on('data', (data) => {
            console.log(chalk.yellow(`[pid: ${p.pid}] ${data}`));
         });
         p.stderr.on('data', (data) => {
            console.log(chalk.redBright(`${data}`));
         });
         p.on('close', (code) => {
            core_info.process = undefined;
            core_info.process_launching = false;
            core_info.dont_launch = true;
            core_info.status = "terminated";
            core_info.exit_code = (code == null ? "unknown" : (code == undefined ? "unknown": code.toString() ) );
            console.log(chalk.red(`[sim core terminated][pid: ${p.pid}][exit code: ${core_info.exit_code}] "${core_info.src.name}"`));
         });
        core_info.process = p;
        setTimeout(() => {core_info.process_launching=false;},3000);
        //Overview generation
        let overview_gen_script = "";
        let modules = "";
        for(let i = 0; i != core_info.src.pkg_info.modules.length ; ++i){
          let m = core_info.src.pkg_info.modules[i];
          modules += " " + m;
        }

        //overview page
        fs.writeFileSync(path.join(sim_nodes_root,core_info.src.path,"make_report.ceps"), 
                                   'make_stddoc{out{"overview.ejs";};img_path_prefix{"img/'+core_info.src.pkg_info.uri+'/";}; };');
        overview_gen_script += "cp make_svgs.sh "+path.join(sim_nodes_root,core_info.src.path)+"\n";
        overview_gen_script += "cp basic_style.ceps "+path.join(sim_nodes_root,core_info.src.path)+"\n";
        overview_gen_script += "cp dot_props.ceps "+path.join(sim_nodes_root,core_info.src.path)+"\n";
        overview_gen_script += 'cd '+path.join(sim_nodes_root,core_info.src.path)+"\npwd\n";        
        overview_gen_script += '../../server/ceps ../../server/'+dbc_lexer+
                               ' ../../.ceps/prelude.ceps basic_style.ceps dot_props.ceps '+modules +
                               ' ../../server/'+ dbc_importer +
                               ' --ignore_simulations --dot_gen --dot_gen_one_file_per_top_level_statemachine --post_processing make_report.ceps\n';
        overview_gen_script += "./make_svgs.sh\n";
        overview_gen_script += "mkdir -p ../../server/public/img/"+core_info.src.pkg_info.uri+"\n";
        overview_gen_script += "cp *.svg ../../server/public/img/"+core_info.src.pkg_info.uri+"\n";
        

        fs.writeFileSync("gen_overview_"+core_info.src.pkg_info.uri+".sh",overview_gen_script);
        let overview_gen_process = spawn("sh",["gen_overview_"+core_info.src.pkg_info.uri+".sh"]);
         overview_gen_process.stdout.on('data', (data) => {
            console.log(chalk.yellow(`${data}`));
         });
         overview_gen_process.stderr.on('data', (data) => {
            console.log(chalk.yellow(`${data}`));
         });
         overview_gen_process.on('close', (code) => {
            console.log(chalk.yellow("gen_overview_"+core_info.src.pkg_info.uri+".sh exited with code "+`${code}`));
         });
         //static can layer documentation
         {
          try{   
            let t = path.join(sim_nodes_root,core_info.src.path,"views");
            try{fs.accessSync(t);} catch (err){fs.mkdirSync(t);}
            t = path.join(sim_nodes_root,core_info.src.path,"views","documentation");
            try{fs.accessSync(t);} catch (err){fs.mkdirSync(t);}
            t = path.join(sim_nodes_root,core_info.src.path,"views","documentation","can_communication_layer");
            try{fs.accessSync(t);} catch (err){fs.mkdirSync(t);}

            cs = make_call_sequence(core_info,{package_info : false,start_publisher : false,start_ws_api : false });
            cs.push("--ignore_simulations");
            cs.push("--dump_stddoc_canlayer");
            cs.push("-o");
            cs.push(path.join(sim_nodes_root,core_info.src.path,"views","documentation","can_communication_layer","all.ejs"));           
            
            let proc = spawn_and_trace_ceps_process("Process Building CAN Documentation ("+core_info.uri+")" ,cs);
            console.log(chalk.bold(`[spawned sim core][pid: ${proc.pid}]${ceps_executable} `,cs.join(" ")));
          } catch (err){
            console.log(chalk.yellow(`***Warning: Failed to create static can layer documentation. (Simulation ${core_info.uri})`));
          }                  
         }
     } else if (core_info.ws === undefined && !core_info.process_launching){         
         core_info.ws = new WebSocket(core_info.url);
         core_info.ws.on("error", () => {
             core_info.ws=undefined;
             console.log("Simulation Core '"+core_info.name+"'@"+core_info.uri+" offline.");
            } );
         core_info.ws.on("open", () => {sim_core_init(core_info);} );
         core_info.ws.on("close", () => {
             core_info.ws=undefined;
             console.log("Simulation Core '"+core_info.name+"'@"+core_info.uri+" offline.");
            } );
     }
    }
}
function get_doc_canlayer_all(core_info){
 return path.join(sim_nodes_root,core_info.src.path,"views","documentation","can_communication_layer","all.ejs");
}

let app = express();
app.set('trust proxy', true);
let publicPath = path.resolve(__dirname, "public");

app.set("views", path.resolve(__dirname, "views"));
app.set("view engine", "ejs");

app.use(express.static(publicPath));

app.get("/", function(req, res) {
    let se = get_streaming_endpoint_by_ip(req.socket.remoteAddress);
    
    res.render("index",{ streaming_endpoint:se,
         streaming_endpoint_host: se == undefined ? undefined : se.host,
        streaming_endpoint_port: se == undefined ? undefined : se.port,
                         page_title:"Home",
                         sim_cores : sim_cores,
                         sim_core : undefined,
                         command_ws_url:command_ws_url,
                        hub_url : "ws://"+master_hub_host+":"+master_hub_port_wsapi });
});

app.get(/^\/(signaldetails__([0-9]+)__([0-9]+))|(\w*)$/, function(req, res,next) {
    let se = get_streaming_endpoint_by_ip(req.socket.remoteAddress);
 if (req.params[3] != undefined) {
    let score = get_sim_core_by_uri(req.params[3]);
    if (score != undefined) {
         if (se != undefined) {
            /* console.log("!!!!!!!!");
             let ws_se = new WebSocket("ws://"+se.host+":"+se.port);
             ws_se.on("open",() => {
                 console.log("Connected @"+se.host+":"+se.port);
             });
             ws_se.on("error",()=>{});*/
         }

         res.render("sim_main",{ streaming_endpoint:se,
            streaming_endpoint_host: se == undefined ? undefined : se.host,
            streaming_endpoint_port: se == undefined ? undefined : se.port,
                                 page_title: score.name,
                                 sim_core : score, 
                                 command_ws_url:command_ws_url,
                                 sim_nodes_root:sim_nodes_root }); 
    } else next();
                                
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
   
   res.render("signal_details",{ streaming_endpoint:se,
                                    streaming_endpoint_host: se == undefined ? undefined :se.host,
                                    streaming_endpoint_port: se == undefined ? undefined : se.port,
                                 page_title: score.name +"-"+ signalname,
                                 sim_core : score,
                                 signal:sig,
                                 signal_ws:score.signal_url});   
 }
});

app.get(/^\/doc_canlayer_all__([0-9]+)$/, function(req, res,next) {
    let se = get_streaming_endpoint_by_ip(req.socket.remoteAddress);
    let score_idx = req.params[0];
    let score = undefined;
    for(let s of sim_cores) if (s.index == score_idx){ score=s;break; }
    if (score === undefined) {res.status=404;res.send("404");return;} 
    res.render("doc_canlayer_all",{streaming_endpoint:se,
        streaming_endpoint_host: se == undefined ? undefined : se.host,
        streaming_endpoint_port: se == undefined ? undefined : se.port,
        page_title: score.name +"-"+ "CAN Layer Documentation",
        sim_name : score.name,
        sim_core : score,
        doc_canlayer_all : get_doc_canlayer_all(score)
    });   
});

app.get("/:sim/controlpanels/:panel", function(req, res, next) {
    let se = get_streaming_endpoint_by_ip(req.socket.remoteAddress);
   let sim_core = get_sim_core_by_uri(req.params.sim);
   if (sim_core === undefined) {res.status=404;res.send("404");return;}
   let f = sim_core.get_view_path(req.params.panel);
   if (f === undefined) {res.status=404;res.send("404");return;}
   res.render("controlpanel",{ streaming_endpoint:se,
                               streaming_endpoint_host: se == undefined ? undefined : se.host,
                               streaming_endpoint_port: se == undefined ? undefined : se.port,
                               panel_loc:"../"+f,page_title:"xxx", 
                               panel_name: req.params.panel , 
                               sim_core : sim_core, 
                               signal_ws : sim_core.signal_url,
                               command_ws_url:command_ws_url});    
});

function move_sim_core_to_trash_by_name(n){
    let i = 0;
    let sc = undefined;
    for (let i = 0; i!=sim_cores.length;++i){
        if (n != sim_cores[i].name) continue;
        sc = sim_cores[i];
        sim_cores.splice(i,1);
        break;
    }
    if (sc != undefined){
        let from = path.join(sim_nodes_root,sc.src.path);
        let to_ = path.join(sim_nodes_trash,sc.src.path);
        let to = undefined;
        ctr = 0;
        for(;;++ctr){
            if (ctr == 0)
             if (!fs.existsSync(to_)){ to = to_;break;}
             else continue;
            to = to_ + ctr.toString();
            if (!fs.existsSync(to)) break;            
        }
        if (to != undefined)fs_mv(from,to,{mkdirp:true},(err)=>{});
    }
}

function add_modify_files_simulation(client_msg,ws){
    //ws.send(JSON.stringify({ok:true}));
    for(sim_core of sim_cores){
        if (sim_core.name != client_msg.name) continue;
        if (sim_core.process != undefined) process.kill(sim_core.process.pid);
        if (sim_core.ws != undefined) sim_core.ws.close();
        sim_core.ws = undefined;
        function add_modify (){
            if (sim_core.process != undefined) {setTimeout(add_modify,500); return;}
            function module_already_exists(n){
                for(let i = 0; i!=sim_core.src.pkg_info.modules.length;++i)
                    if (sim_core.src.pkg_info.modules[i] == n) return true;
                return false;
            }
            for(let i = 0;i != client_msg.file_names.length;++i){
               if(!module_already_exists(client_msg.file_names[i]))
                sim_core.src.pkg_info.modules.push(client_msg.file_names[i]);
        
               fs.writeFileSync(path.join(sim_nodes_root,sim_core.src.path,client_msg.file_names[i]),client_msg.data[i]);
            }
            fs.writeFileSync(path.join(sim_nodes_root,sim_core.src.path,"package.json"),JSON.stringify(sim_core.src.pkg_info));
            sim_core.dont_launch = false;
            function ack() { if(sim_core.process==undefined){setTimeout(ack,1000);return;} ws.send(JSON.stringify({ok:true})); }
            setTimeout(ack,1000);
        }
        setTimeout(add_modify,3000);
    }
}

function create_fibex_based_simulation(client_msg,ws){
    //let sim_src = make_sim_src_info_given_sim_dir(directory,subd,(s)=>{return uniquify_name(r,s);},(s)=>{return uniquify_uri(r,s);});
    fs.readdir(sim_nodes_root,(err,files)=>{
        if (err){ws.send(JSON.stringify({ok:false}));return;}
        function find_match (s){for(let i = 0; i != files.length;++i)if(files[i]==s)return i;return -1;}
        let i=0;
        let s = client_msg.name;
        s = s.replace(/\s/g,"_").replace(/\//g,"").replace(/\./g,"").replace(/:/g,"");
        let subd=s;
        while(0 <= find_match(subd)){
            ++i;
            subd = s+"_"+i.toString();
        }
        fs.mkdir(path.join(sim_nodes_root,subd),(err)=>{
          if (err){ws.send(JSON.stringify({ok:false}));return;}
          let pkg_json_sent = false;
          let driver_ceps_sent = false; 
          for(let i = 0; i!=client_msg.file_names.length;++i){
              if(client_msg.file_names[i] === "package.json") pkg_json_sent = true;
              if(client_msg.file_names[i] === "driver.ceps") driver_ceps_sent = true;
              fs.writeFileSync(path.join(sim_nodes_root,subd,client_msg.file_names[i]),client_msg.data[i]);
          }
          ws.send(JSON.stringify({ok:true}));
          if (!pkg_json_sent){
              let jsn = {name:client_msg.name,modules:[]};
              for(let i = 0; i!=client_msg.file_names.length;++i){
                  if (client_msg.file_names[i].match(/xml$/)) jsn.modules.push(client_msg.file_names[i]);
              }
              for(let i = 0; i!=client_msg.file_names.length;++i){
                  if (!client_msg.file_names[i].match(/xml$|driver\.ceps/)) jsn.modules.push(client_msg.file_names[i]);
              }
              jsn.modules.push("driver.ceps");  
              fs.writeFileSync(path.join(sim_nodes_root,subd,"package.json"),JSON.stringify(jsn));
          }
          if(!driver_ceps_sent) fs.writeFileSync(path.join(sim_nodes_root,subd,"driver.ceps"),"Simulation{};");
          let sim_src = make_sim_src_info_given_sim_dir(sim_nodes_root,subd,(s)=>{return uniquify_name(sim_srcs,s);},(s)=>{return uniquify_uri(sim_srcs,s);});
          if (sim_src == undefined)
            ws.send(JSON.stringify({ok:false}));
          else
            setTimeout( () => { 
                ws.send(JSON.stringify({ok:true}));
                instantiate_sim_info([sim_src],sim_cores);
                let sim_core = sim_cores[sim_cores.length-1];
                setTimeout( () => {
                    if (sim_core.process != undefined) ws.send(JSON.stringify({ok:true,uri:sim_core.uri}));
                    else {
                        ws.send(JSON.stringify({ok:false}));
                    }
                }, 3000);
            }, 3000 );
          console.log (sim_src);
        });
    });
}

function create_empty_simulation(client_msg,ws){
    fs.readdir(sim_nodes_root,(err,files)=>{
        if (err){
            ws.send(JSON.stringify({ok:false}));
            return;
        }
        
        function find_match (s){
            for(let i = 0; i != files.length;++i)
             if(files[i]==s)return i;
            return -1;
        }
        
        let i=0;
        let s = client_msg.name;

        s = s.replace(/\s/g,"_").replace(/\//g,"").replace(/\./g,"").replace(/:/g,"");
        let subd=s;
        while(0 <= find_match(subd)){
            ++i;
            subd = s+"_"+i.toString();
        }
        
        fs.mkdir(path.join(sim_nodes_root,subd),(err)=>{
          if (err){
              ws.send(JSON.stringify({ok:false}));
              return;
          }

          let pkg_json_sent = false;
          let driver_ceps_sent = false; 
          
          for(let i = 0; i!=client_msg.file_names.length;++i){
              if(client_msg.file_names[i] === "package.json") pkg_json_sent = true;
              if(client_msg.file_names[i] === "driver.ceps") driver_ceps_sent = true;
              fs.writeFileSync(path.join(sim_nodes_root,subd,client_msg.file_names[i]),client_msg.data[i]);
          }
          
          if (!pkg_json_sent){
              let jsn = {name:client_msg.name,modules:[]};
              for(let i = 0; i!=client_msg.file_names.length;++i){
                  if (client_msg.file_names[i].match(/xml$/)) jsn.modules.push(client_msg.file_names[i]);
              }
              for(let i = 0; i!=client_msg.file_names.length;++i){
                  if (!client_msg.file_names[i].match(/xml$|driver\.ceps/)) jsn.modules.push(client_msg.file_names[i]);
              }
              jsn.modules.push("driver.ceps");  
              fs.writeFileSync(path.join(sim_nodes_root,subd,"package.json"),JSON.stringify(jsn));
          }
          
          if(!driver_ceps_sent) fs.writeFileSync(path.join(sim_nodes_root,subd,"driver.ceps"),"Simulation{};");
          
          let sim_src = make_sim_src_info_given_sim_dir(sim_nodes_root,subd,(s)=>{return uniquify_name(sim_srcs,s);},(s)=>{return uniquify_uri(sim_srcs,s);});
          if (sim_src == undefined)
            ws.send(JSON.stringify({ok:false}));
          else{
            instantiate_sim_info([sim_src],sim_cores);
            let sim_core = sim_cores[sim_cores.length-1];
            ws.send(JSON.stringify({ok:true}));
          }
        });
    });
}


const ws_command = new WebSocket.Server({port: command_port});
ws_command.on("connection", function connection(ws){
    ws.on("message",function incoming(message){
        let msg = JSON.parse(message);
        let sim_src = undefined;
        if (msg.cmd == "create_fibex_based_simulation"){
           create_fibex_based_simulation(msg,ws);       
          //ws.send(JSON.stringify({ok:true}));  
        } else if (msg.cmd == "add_files"){
           add_modify_files_simulation(msg,ws);       
        } else if (msg.cmd == "upload_ctrl_panel_data"){
            let ctrl_panel_name = msg.ctrl_panel_name;
            let sim_core_uri = msg.sim_core_uri;
            let data = msg.data;
            console.log(msg);
            let sim_core = get_sim_core_by_uri(sim_core_uri);
            if (sim_core != undefined){
                try{
                    fs.writeFileSync(path.join(sim_nodes_root,sim_core.src.path,"views",ctrl_panel_name+".ejs"),data);
                    console.log("ok");
                  } catch (err) {
                    console.log(err);
                    ws.send(JSON.stringify({ok:false,reason:"Error Code: '"+err.code+"'"}));return;  
                  }
                  ws.send(JSON.stringify({ok:true}));
            }
        } else if (msg.cmd == "create_control_panel"){
            let ctrl_panel_name = msg.name;
            let sc = get_sim_core_by_name(msg.sim_name);
            if (sc == undefined){ws.send(JSON.stringify({ok:false,reason:"Simulation doesn't exist."}));return;}
            try{
              fs.mkdirSync(path.join(sim_nodes_root,sc.src.path,"views"));
            } catch (err) {
              console.log(err);
              if (err.code != "EEXIST"){
                ws.send(JSON.stringify({ok:false,reason:"Error Code: '"+err.code+"'"}));return;  
              }
            } 
            try{
              fs.writeFileSync(path.join(sim_nodes_root,sc.src.path,"views",ctrl_panel_name+".ejs"),"<p>Hello, World!</p>");
            } catch (err) {
              console.log(err);
              ws.send(JSON.stringify({ok:false,reason:"Error Code: '"+err.code+"'"}));return;  
            }
            ws.send(JSON.stringify({ok:true,uri:sc.uri+"/controlpanels/"+ctrl_panel_name}));
        } else if (msg.cmd == "info"){
            let r = { sim_cores:[]};
            for (let s of sim_cores){
                r.sim_cores.push({
                    name : s.name,
                    status : s.status
                });
            }
            ws.send(JSON.stringify({ok:true,result: r }));
        } else if (msg.cmd == "kill"){
            let sc = get_sim_core_by_name(msg.name);
            if (sc == undefined) {ws.send(JSON.stringify({ok:true}));return;}
            sc.process.kill();
            sc.status = "terminating";
            ws.send(JSON.stringify({ok:true}));        
        } else if (msg.cmd == "launch"){
            let sc = get_sim_core_by_name(msg.name);
            if (sc == undefined) {ws.send(JSON.stringify({ok:true}));return;}
            sc.dont_launch = false;
            sc.status = "launching";
            ws.send(JSON.stringify({ok:true}));        
        } else if (msg.cmd == "delete"){
            let sc = get_sim_core_by_name(msg.name);
            if (sc == undefined) {ws.send(JSON.stringify({ok:true}));return;}
            if (sc.process != undefined) sc.process.kill();
            move_sim_core_to_trash_by_name(msg.name);
            ws.send(JSON.stringify({ok:true}));        
        } else if (msg.cmd == "create_simulation"){
            create_empty_simulation(msg,ws);                    
        }
    });
});

function check_for_views(){
   sim_cores.forEach( (sim_core) => {
    fs.readdir(path.join(sim_nodes_root,sim_core.src.path,"views"),(err,files) => {
        sim_core.src.views = [];
        if (err) return;
        for(f of files){
            let r = f.match(/^(\w(\w|\s)*)\.ejs$/);
            if(!r || !r[1]) continue;
            sim_core.src.views.push(r[1]);
        }
    });
   } );
}


sim_srcs = walk_dir_and_fetch_sim_src_infos("../sim_nodes");
instantiate_sim_info(sim_srcs,sim_cores);
check_for_views();
check_remote_sim_cores();
setInterval(check_remote_sim_cores,500);
setInterval(check_for_views,2000);
http.createServer(app).listen(3000);
