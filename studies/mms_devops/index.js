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
const username = require("username");
const host_name = os.hostname();

const ceps_cmd_ = "./ceps";
let fetch_rollouts_ceps_api_port_base = 10100;
let fetch_rollouts_ceps_api_port = fetch_rollouts_ceps_api_port_base;
let fetch_rollouts_ceps_api_port_window = 899;

const ws_api_base = 11001;
const command_port = ws_api_base - 1;
let ws_api_next = 0;
const ws_api_max = 1000;

const ROLLOUT_READY = 0;
const ROLLOUT_STARTED_PENDING = 1;
const ROLLOUT_STARTED = 2;
const ROLLOUT_STOP_PENDING = 3;
const ROLLOUT_FAILED = 4;
const ROLLOUT_DONE = 5;

const global_conf = require('./rollaut.json');
const MAX_RECONNECTS_DURING_CEPS_SPAWN = 100;
const DB_CHECK_INTERVAL_MS = 10000;


let connections = [

];

let connections_with_ceps_cores = {
  //via pid
};

function simplifyceps2json(o,m){

    function is_attr(e){
        if (e.type == "struct" && e.content.length == 1 && e.content[0].type == undefined) return true;
        return false;
    }

    if (m.content == undefined){
        if (m.type == "binop"){
            
        }
    } else for(let i = 0; i != m.content.length;++i){
        let e = m.content[i];
        if (!is_attr(e)){
            let field = e.name;
            if (o[field] == undefined) o[field] = [];
            let t = {};
            simplifyceps2json(t,e);
            o[field].push(t);
        } else {
            o[e.name]  = e.content[0];
        }
    }
}


function broadcast(msg){
 //let b = false;
 try{
    for(let i = 0; i != connections.length;++i){
    if (connections[i].readyState != connections[i].OPEN) continue;
    //if(!b){console.log(`broadcasting: ${msg}`);console.log();b=true;}
    connections[i].send(msg);
    }
 } catch (err) {

 }

}

let rollouts = [
    /*
    {id:"1234",state::ROLLOUT_...,when:"28-03-2019 00:00:00",steps:[{name:"abc"},{name:"def"}],markets:[{name:"m1",sap_code:"123"}]}
    */
];

function save_rollout(rollout){
    if (rollouts.length == 0){
     rollouts.push(rollout);
     return rollouts[rollouts.length-1];
    } else {
     for(let i = 0; i != rollouts.length;++i){
        if (rollout.id != rollouts[i].id) continue;
        rollouts[i] = rollout;
        return rollouts[i];
     }
    }
}

function lookup_rollout(rollout){
    for(let i = 0; i != rollouts.length;++i){
        if (rollout.id == rollouts[i].id) return rollouts[i];
    }
    return rollout;
}

function get_rollout_by_name(rollout_name){
    for(let i = 0; i != rollouts.length;++i){
        if (rollout_name == rollouts[i].name) return rollouts[i];
    }
    return undefined;
}

function get_rollout_by_id(rollout_id){
    for(let i = 0; i != rollouts.length;++i){
        if (rollout_id == rollouts[i].id) return rollouts[i];
    }
    return undefined;
}

function get_next_ws_api_port(){
    let p = ws_api_base + ws_api_next;
    ws_api_next = (ws_api_next+1)%ws_api_max;
    return p;
}

function extract_rollout_data_from_webservice_result(m){
    function extract_steps(v){
        let r = [];
        for(let i = 0; i != v.length;++i){
            if (v.type != undefined) continue;
            r.push({ name: v[i]} );
        }
        return r;        
    }
    function extract_markets(v){
        let r = [];
        for(let i = 0; i != v.length;++i){
            if (v.type != undefined) continue;
            r.push({ name: v[i] } );
        }
        return r;        
    }
    function extract_rollout(dest,v){
        dest.name = v[0];
        for(let i = 1; i < v.length;++i){
            if (v[i].type != "struct") continue;
            if (v[i].name == "id") dest.id = v[i].content[0];
            else if (v[i].name == "when") dest.when = v[i].content[0];
            else if (v[i].name == "steps")
             dest.steps = extract_steps(v[i].content);
            else if (v[i].name == "markets")
             dest.markets = extract_markets(v[i].content);             
        }
    }
    let r = [];
    for(let i = 0; i != m.length;++i){
        if (m[i].type == "struct" && m[i].name == "rollout_"){
            let rollout = {state:ROLLOUT_READY};
            extract_rollout(rollout,m[i].content);            
            r.push(rollout);
        }
    }
    return r;
}


function ceps_cmd_add_param_ws_api(v,port){
    v.push("--ws_api");v.push(port);return v;
}

function ceps_cmd() {return ceps_cmd_;}



let app = express();
app.set('trust proxy', true);
let publicPath = path.resolve(__dirname, "web");
app.set("views", path.resolve(__dirname, "web/views"));
app.set("view engine", "ejs");
app.use(express.static(publicPath));

app.get("/tables", function(req, res){
    res.render("tableview",{ 
        server_name : host_name,
        command_port : command_port  }
    );
});

app.get("/rollout_status", function(req, res) {
    let rollout_name = req.query.rollout;
    let rollout_market = req.query.market;
    let plain_market_url_list = req.query.plain_text_list_of_markets;

    let rollout = get_rollout_by_name(rollout_name);
    if (rollout == undefined) {
        rollout={name:rollout_name};
        res.render("404",{ 
            command_port : command_port,rollout:rollout  }
        );
    } else {
        if (plain_market_url_list != undefined){
            res.type("text/plain");

            let r = "";

            for(let i = 0; i != rollout.markets.length;++i){
                let e = rollout.markets[i];
                let name = "";
                if (e.name == undefined || e.name.type != undefined) continue;
                r += `http://${req.headers.host}/rollout_status?rollout=${encodeURI(rollout_name)}&market=${encodeURI(e.name)}
`;
            }
            res.send(r);
        }
        else if (rollout_market == undefined) res.render("index",{ 
                hostname_ceps_service: host_name, 
                ceps_api_port : rollout.ws_api,
                server_name : host_name,
                command_port : command_port,
                rollout      : rollout,
                rollout_market : ""
            }
        ); else res.render("index",{ 
                hostname_ceps_service: host_name, 
                ceps_api_port : rollout.ws_api,
                server_name : host_name,
                command_port : command_port,
                rollout      : rollout,
                rollout_market : rollout_market
            }
        );
    }
});

app.get("/", function(req, res) {
    res.render("rollout_admin",{ 
        server_name : host_name,
        command_port : command_port  }
    );
});

let get_planned_rollouts = undefined;


function merge_rollouts(new_rollouts,removed_rollouts){
    
    let v_new = [];
    let changed = false;
    let t = [];

    for(let i = 0; i!=rollouts.length;++i) rollouts[i].changed = false;
    for(let i = 0; i!=new_rollouts.length;++i){
        let r = get_rollout_by_id(new_rollouts[i].id);
        if (r == undefined){
            v_new.push(new_rollouts[i]);
        } else if (r.state == ROLLOUT_READY){
            r.changed = false;
            if (r.name != new_rollouts[i].name || 
                r.when != new_rollouts[i].when || 
                JSON.stringify(r.steps) != JSON.stringify(new_rollouts[i].steps) ||
                JSON.stringify(r.markets) != JSON.stringify(new_rollouts[i].markets) ){
                changed = r.changed = true;
                r.name = new_rollouts[i].name;
                r.when = new_rollouts[i].when;
                r.steps = new_rollouts[i].steps;
                r.markets = new_rollouts[i].markets;
                t.push(r);
            }            
        }
    }

    for(let i = 0; i!=rollouts.length;++i){
        if (rollouts[i].state != ROLLOUT_READY) {t.push(rollouts[i]);continue;}
        let found = false;
        for(let j = 0; j != new_rollouts.length;++j)
        {
            if (rollouts[i].id == new_rollouts[j].id) {found = true;break;}
        }
        if (found) {if (!rollouts[i].changed) t.push(rollouts[i]);}
        else if (removed_rollouts != undefined) removed_rollouts.push(rollouts[i]);
        if (!found) changed = true;
    }

    if (v_new.length > 0){
        for(let i = 0; i!=v_new.length;++i){
            changed = v_new[i].changed = true;
            t.push(v_new[i]);
        }
    }

    rollouts = t;
    //console.log(`CHANGED = ${changed}`);

    return changed;
}

let rollout_fetch_action_active = false;

let fetch_planned_rollouts = function (back_channel,callback){

    //if (rollout_fetch_action_active) return;
    //rollout_fetch_action_active = true;

    if (rollouts != undefined && rollouts.length>0 && back_channel != undefined){
        if (back_channel != undefined) back_channel.send(JSON.stringify(rollouts));
        return ;
    }
    let params = undefined;
    if (global_conf.rollouts == undefined)  
        params = ceps_cmd_add_param_ws_api(
                                [`db_rollout_dump.ceps`,`db_descr/gen.ceps`,`drivers/empty_simulation.ceps`],
                                `${fetch_rollouts_ceps_api_port}`);
    else 
        params = ceps_cmd_add_param_ws_api(
        [global_conf.rollouts,`drivers/empty_simulation.ceps`],
        `${fetch_rollouts_ceps_api_port}`);

    let current_ws_api_port = fetch_rollouts_ceps_api_port;
    fetch_rollouts_ceps_api_port = fetch_rollouts_ceps_api_port + 1;
    if (fetch_rollouts_ceps_api_port > fetch_rollouts_ceps_api_port_base + fetch_rollouts_ceps_api_port_window)
    fetch_rollouts_ceps_api_port = fetch_rollouts_ceps_api_port_base;


    let cproc = spawn(ceps_cmd(),params);                        
    log_debug(`fetch_planned_rollouts()`,`Spawned Child Process pid=${cproc.pid}\nCommand: ${ceps_cmd()} ${params.join(" ")}`);
    let proc_down = false;
    cproc.stdout.on('data', (data) => {
        //console.log(chalk.yellow(`${data}`));
    });
    cproc.stderr.on('data', (data) => {
        log_err(`fetch_planned_rollouts()/cepS core pid=${cproc.pid}`,`${data}`);
    });
    cproc.on('close', (code) => {
        proc_down = true;
        //try{process.kill(cproc.pid);}catch(err){}
        log_debug(`fetch_planned_rollouts()/cepS core pid=${cproc.pid}`,`exited with code ${code}`);
    });

    let fetch_proc = function () {
        //let current_ws_api_port = fetch_rollouts_ceps_api_port;
        log_debug(`fetch_planned_rollouts()/fetch_proc()`,`Trying to establish a websocket connect to localhost:${current_ws_api_port} pid=${cproc.pid}`);
        let ws_ceps_api = new WebSocket(`ws://localhost:${current_ws_api_port}`);
        ws_ceps_api.on("error", (err) => {
            log_err("fetch_proc()",`Connect to ws://localhost:${current_ws_api_port} failed pid=${cproc.pid}`);
            if (!proc_down) setTimeout(fetch_proc,1000);
            else callback(err);
        });
        ws_ceps_api.on("open", () => {
            ws_ceps_api.on("message", function (msg){
                log_debug(`fetch_planned_rollouts()/fetch_proc()`,`Received a reply from localhost:${current_ws_api_port} pid=${cproc.pid}`);
                let m = JSON.parse(msg).sresult;
                let removed_rollouts = [];
                new_rollouts = extract_rollout_data_from_webservice_result(m);
                let changed = merge_rollouts(new_rollouts,removed_rollouts);
                if (changed){
                    log_debug(`fetch_planned_rollouts()/db update`,`Changes in DB detected`);
                    for(let i = 0; i != rollouts.length;++i){
                    let rollout = rollouts[i];
                     if (rollout.changed != undefined && rollout.changed){
                        log_debug(`fetch_planned_rollouts()/db update`,`Rollout id=${rollout.id} changed, trigger broadcast. pid=${cproc.pid}`);
                      broadcast(JSON.stringify({reply:"ok",rollout:rollout}));
                     }
                 }
                 for(let i = 0; i != removed_rollouts.length;++i){
                    let rollout = removed_rollouts[i];
                    let r = {id:rollout.id};
                    log_debug("","Removing Rollout with id = "+r.id);
                    broadcast(JSON.stringify({reply:"ok",rollout:r}));
                 }
                } else log_debug("merge_rollouts: no changes detected");
                try{if (back_channel != undefined) back_channel.send(JSON.stringify(rollouts));}catch(err){}
                log_debug(`fetch_planned_rollouts()/fetch_proc()`,`Shutting down of process with pid=${cproc.pid}`);
                let kill_proc = ()=> {
                    try{process.kill(cproc.pid);}catch(err){
                        log_err("fetch_planned_rollouts()/fetch_proc():kill_proc",`Failed to kill process with pid=${cproc.pid}. Retry in 1 second.`);
                        setTimeout(kill_proc,2000);
                    }
                };
                kill_proc();
                if (callback != undefined) callback(undefined);
            });
            ws_ceps_api.send("QUERY root.rollout_;");         
        } );
        ws_ceps_api.on("close", () => {} );
       };
       setTimeout(fetch_proc,6000);
}

function make_timestamp(){
    function two_digits(n){
        if (n < 10) return "0"+n.toString();
        else return n.toString();
    }

    let t = new Date(Date.now());
    let timestamp = `${t.getFullYear()}-${two_digits(t.getMonth()+1)}-${two_digits(t.getDate())} ${two_digits(t.getHours())}:${two_digits(t.getMinutes())}:${two_digits(t.getSeconds())}`;
    return timestamp;
}

function log_info(who,msg){
    console.log(`${make_timestamp()} [${who}] ${msg}`);
}
function log_err(who,msg){
    console.log(chalk.red(`${make_timestamp()} [${who}] ${msg}`));
}
function log_debug(who,msg){
    if (global_conf.log_debug != undefined && global_conf.log_debug) console.log(chalk.yellow(`${make_timestamp()} [${who}] ${msg}`));
//   fs.appendFileSync("log.txt",`${make_timestamp()} ${who} ${msg}
//`);

}

const CEPS_INSTANCE_SPAWNING = 1;
const CEPS_INSTANCE_EXITED   = 2;
const CEPS_INSTANCE_WSAPI_CONNECTED = 4;
const CEPS_INSTANCE_WSAPI_CONNECTING = 8;
const CEPS_INSTANCE_COMPLETE = 16;
const CEPS_INSTANCE_RUNNING_ROLLOUT = 32;

let ceps_instances = [

];

function watch_ceps_instance(info){
    if (ceps_instances.length == 0) {ceps_instances.push(info);return ceps_instances.length - 1;}
    for(let i = 0; i != ceps_instances.length; ++i)
    {
        if (ceps_instances[i].status == CEPS_INSTANCE_COMPLETE){
            ceps_instances[i] = info;
            return i;
        }
    }
    ceps_instances.push(info);
    return ceps_instances.length - 1;
}

setInterval( () => {
    for(let i = 0; i != ceps_instances.length; ++i)
    {
        let info = ceps_instances[i];
        if (info == undefined) continue;
        let proc_idx = i;
        let rollout = info.rollout;
        log_debug("cepS Core Watch",
        `'${rollout.name}': spawn=${(info.status & CEPS_INSTANCE_SPAWNING) != 0},terminated:${ (info.status & CEPS_INSTANCE_EXITED) != 0},`+
        `connecting:${ (info.status & CEPS_INSTANCE_WSAPI_CONNECTING) != 0},connected:${ (info.status & CEPS_INSTANCE_WSAPI_CONNECTED) != 0} `);

        if (info.status & CEPS_INSTANCE_COMPLETE) continue;
        if (info.status & CEPS_INSTANCE_RUNNING_ROLLOUT){
            if (info.status & CEPS_INSTANCE_EXITED){
                connections_with_ceps_cores[rollout.pid.toString] = undefined;
                clearInterval(info.periodic_status);
                info.periodic_status=undefined;
                rollout.pid = info.proc_info.pid = undefined;
                rollout.ws_api = info.ws_api_port = get_next_ws_api_port();
                info.ws_ceps_api = undefined;
                info.status = CEPS_INSTANCE_SPAWNING | CEPS_INSTANCE_EXITED;
            }
        } else if (info.status & CEPS_INSTANCE_SPAWNING){
            //log_debug("cepS Core Watch",`Spawning ${rollout.name}`);
            if (info.status & CEPS_INSTANCE_EXITED){
                info.status = CEPS_INSTANCE_SPAWNING;
                info.ws_api_port = get_next_ws_api_port();
                let cmd_args2 = info.cmd_args.concat(["--ws_api",`${info.ws_api_port}`]);
                let ceps_process = undefined;

                try{
                    ceps_process = spawn(info.cmd,cmd_args2,info.options);
                } catch (err){
                    try{log_err("cepS Core Watch",err.stack);}catch(err){}
                }

                info.proc_info = ceps_process;
                log_info("watch cepS Core",`Restarting ${info.cmd} ${cmd_args2.join(" ")}`);    
                log_info("watch cepS Core",`Spawned Child Process pid=${ceps_process.pid}`);
    
                ceps_process.stdout.on('data', (data) => {
                    log_debug(`launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${data}`);
                });
                ceps_process.stderr.on('data', (data) => {
                    log_err(`launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${data}`); 
                });
                ceps_process.on('close', (code) => {
                    //try{process.kill(ceps_process.pid);}catch(err){}
                    ceps_instances[proc_idx].status = CEPS_INSTANCE_EXITED | CEPS_INSTANCE_SPAWNING;
                    connections_with_ceps_cores[ceps_process.pid] = undefined;
                    log_debug(`launch_rollout()/cepS core running,pid=${ceps_process.pid} '${rollout.name}'`,`exited with code ${code}`);    
                });
                ceps_process.on('error', (err) => {
                    try{log_err(`launch_rollout()/cepS core running,pid=${ceps_process.pid} '${rollout.name}'`,err.stack);}catch(err){}                     
                });

            } else if (  !(info.status & CEPS_INSTANCE_WSAPI_CONNECTED) && !(info.status & CEPS_INSTANCE_WSAPI_CONNECTING)){
                log_debug("cepS Core Watch",`Connecting '${rollout.name}'@ws://localhost:${info.ws_api_port}`);
                let ws_ceps_api = new WebSocket(`ws://localhost:${info.ws_api_port}`);
                info.status |= CEPS_INSTANCE_WSAPI_CONNECTING;
                if (info.connect_counter == undefined)
                 info.connect_counter = 1;
                else ++info.connect_counter;
                if (MAX_RECONNECTS_DURING_CEPS_SPAWN <= info.connect_counter){
                    info.connect_counter = 0;
                    try{process.kill(info.proc_info.pid);}catch(err){}
                }                

                ws_ceps_api.on("error", () => {
                    log_debug("cepS Core Watch",`Failed to connect to '${rollout.name}'@ws://localhost:${info.ws_api_port}`);
                    //info.status &= ~(CEPS_INSTANCE_WSAPI_CONNECTING || CEPS_INSTANCE_WSAPI_CONNECTED);
                } );
                ws_ceps_api.on("open", () => {
                    log_debug("cepS Core Watch",`Connection to '${rollout.name}'@ws://localhost:${info.ws_api_port} established. Sending PING`);
                    let f = undefined;
                    ws_ceps_api.on("message", f = function (msg){
                        let m = undefined;
                        try{m = JSON.parse(msg);}catch(err){
                            try{log_err("cepS Core Watch",err.stack);}catch(err){}
                        }
                        if (m!=undefined && m.ok && m.reply == "PONG"){
                            log_debug("cepS Core Watch",`Successfuly connected to '${rollout.name}'@ws://localhost:${info.ws_api_port}`);
                            info.status = CEPS_INSTANCE_WSAPI_CONNECTED;
                            ws_ceps_api.removeEventListener("message",f);
                            ws_ceps_api.on("message", f = function (msg){
                                ws_ceps_api.removeEventListener("message",f);
                                let m = JSON.parse(msg);
                                if (m.ok){
                                    info.status |= CEPS_INSTANCE_RUNNING_ROLLOUT;
                                    ws_ceps_api.on("message", f = function (msg){
                                        rollout.state = ROLLOUT_STARTED;
                                        rollout.pid = info.proc_info.pid;
                                        rollout.ws_api = info.ws_api_port;
                                        rollout.hostname = host_name;
                                        rollout.url = "/rollout_status?rollout="+encodeURIComponent(rollout.name);
                                        rollout.svgs = info.rollout_path_suffix+"__svgs";//encodeURIComponent(rollout.name).replace("%","_")+"__svgs";
                                        //save_rollout(rollout);
                                        try{broadcast(JSON.stringify({reply:"ok",rollout:rollout}));}catch(err){}
                                        connections_with_ceps_cores[rollout.pid.toString] = ws_ceps_api;
                                        info.ws_ceps_api = ws_ceps_api;
                                        ws_ceps_api.removeEventListener("message",f);

                                        info.periodic_status = setInterval(
                                            function(){
                                                try{ws_ceps_api.send("QUERY root.__proc.coverage");}catch(err){
                                                    //log_err(`launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${err}`); 
                                                }
                                            },5000
                                        );
                                        ws_ceps_api.on("message", function(msg){
                                            try{
                                                let reply_ = JSON.parse(msg);
                                                if (!reply_.ok) return;
                                                let reply = {};
                                                if (reply_.sresult == undefined) return;
                                                if (reply_.sresult.coverage == undefined) return;                                                
                                                simplifyceps2json(reply,reply_.sresult.coverage);
                                                rollout.coverage = reply.coverage[0].transition_coverage[0].ratio;
                                                rollout.health = reply.categories[0];                                            
                                                try{broadcast(JSON.stringify({reply:"ok",rollout:rollout}));}catch(err){}
                                            } catch(err) {

                                            }
                                        });
                                        try{ws_ceps_api.send("QUERY root.__proc.coverage");}catch(err){}
                                    });
                                    try{ws_ceps_api.send("QUERY root.rollout;");}catch(err){}
                                }
                                else try{process.kill(info.proc_info.pid);}catch(err){}
                            });
                            ws_ceps_api.send("EVENT @@start");              
                        } else {
                            try{process.kill(info.proc_info.pid);}catch(err){}                            
                        }
                    });
                    try{ws_ceps_api.send("PING");}catch(err){}
                } );
                ws_ceps_api.on("close", () => {
                    log_debug("cepS Core Watch",`Close Connection to '${rollout.name}'@ws://localhost:${info.ws_api_port}`);
                    info.status &= ~(CEPS_INSTANCE_WSAPI_CONNECTING || CEPS_INSTANCE_WSAPI_CONNECTED);
                } );
        
            }
        }
    }
    
},1000);


 function start_ceps_instance (rollout,
                               rollout_path_base,
                               rollout_db_full,
                               ws_api_port,
                               rollout_path_suffix){
    
    let cmd_args = [];

    if (global_conf.skip_worker_sm_creation){
        log_debug(`launch_rollout()`,`Worker State Machines will not be created. global_conf.skip_worker_sm_creation == true`);
        cmd_args = [
            `${rollout_path_base}/globals.ceps`,
            `../${rollout_db_full}`,
            "../db_descr/gen.ceps",
            `${rollout_path_base}/extract_rollout.ceps`,
            "../lib/conf.ceps",
            "../lib/rollout_step.ceps",
            "../transformations/rollout2sm.ceps",
            "../transformations/rollout2watchdogs.ceps",
            "../transformations/driver4rollout_start_immediately_no_worker.ceps",
            "--dot_gen_one_file_per_top_level_statemachine",
            "--dot_gen",
            "--no_file_output",
            "--start_paused"
        ];
    } else cmd_args = [
        `${rollout_path_base}/globals.ceps`,
        `../${rollout_db_full}`,
        "../db_descr/gen.ceps",
        `${rollout_path_base}/extract_rollout.ceps`,
        "../lib/conf.ceps",
        "../lib/rollout_step.ceps",
        "../transformations/rollout2worker.ceps",
        "../transformations/rollout2sm.ceps",
        "../transformations/rollout2watchdogs.ceps",
        "../transformations/driver4rollout_start_immediately.ceps",
        "--dot_gen_one_file_per_top_level_statemachine",
        "--dot_gen",
        "--no_file_output",
        "--start_paused"/*,
        "--sleep_before_ws_api",
        "6000000"*/
    ];

    let cmd_args2 = cmd_args.concat(["--ws_api",`${ws_api_port}`]);

    let ceps_process = undefined;
    try{
        ceps_process = spawn("../ceps",cmd_args2,{cwd:"runs"});
    } catch (err){
        try{log_err(`${rollout.name} - launch_rollout()/cepS core running`,err.stack);}catch(err){}
    }

    let proc_idx = watch_ceps_instance(
        {
            status : CEPS_INSTANCE_SPAWNING,
            proc_info : ceps_process,
            cmd : "../ceps",
            cmd_args : cmd_args,
            options : {cwd:"runs"}, 
            rollout:rollout,
            ws_api_port : ws_api_port,
            rollout_path_base : rollout_path_base,
            rollout_db_full : rollout_db_full,
            rollout_path_suffix:rollout_path_suffix
        }
    );

    log_info(`${rollout.name} - launch_rollout()`,`Executing ../ceps ${cmd_args2.join(" ")}`);    
    log_info(`${rollout.name} - launch_rollout()`,`Spawned Child Process pid=${ceps_process.pid}`);
    
    ceps_process.stdout.on('data', (data) => {
        log_debug(`${rollout.name} - launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${data}`);
    });
    ceps_process.stderr.on('data', (data) => {
        log_err(`${rollout.name} - launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${data}`); 
    });
    ceps_process.on('close', (code) => {
        //try{process.kill(ceps_process.pid);}catch(err){}
        ceps_instances[proc_idx].status |= CEPS_INSTANCE_EXITED;
        connections_with_ceps_cores[ceps_process.pid] = undefined;
        log_debug(`${rollout.name} - launch_rollout()/cepS core running,pid=${ceps_process.pid} '${rollout.name}'`,`exited with code ${code}`);    
    });
    ceps_process.on('error', (err) => {
        try{
            log_err(`${rollout.name} - launch_rollout()/cepS core running,pid=${ceps_process.pid} '${rollout.name}'`,err.stack);
        }catch(err){}
    });
};

function restart_sm(ws,sm,rollout_name){
    let rollout = get_rollout_by_name(rollout_name);
    let worker = sm+"_worker";
    let watch_dog_sm = "watch_dog_"+sm;
    let representational_sm = sm;

    
    for(let i = 0; i != ceps_instances.length; ++i)
    {
        let info = ceps_instances[i];
        //console.log(info);
        let rollout = info.rollout;
        if (rollout.name != rollout_name) continue;
        try{info.ws_ceps_api.send(
`RESTART_STATEMACHINES

${representational_sm}
${worker}
${watch_dog_sm}
`                               );

setTimeout(()=>{
    try{info.ws_ceps_api.send(
`EVENTL

start_rollout_${sm}
`);}catch(err){}       
},2000);


        }catch(err){}
    }
}

function kill_sm(ws,sm,rollout_name){
    let rollout = get_rollout_by_name(rollout_name);
    let worker = sm+"_worker";
    let watch_dog_sm = "watch_dog_"+sm;
    let representational_sm = sm;

    
    for(let i = 0; i != ceps_instances.length; ++i)
    {
        let info = ceps_instances[i];
        //console.log(info);
        let rollout = info.rollout;
        if (rollout.name != rollout_name) continue;
        try{info.ws_ceps_api.send(
`RESTART_STATEMACHINES

${representational_sm}
${worker}
${watch_dog_sm}
`                               );

setTimeout(()=>{
    try{info.ws_ceps_api.send(
`EVENTL

dump_rollout_${sm}
`);}catch(err){}       
},2000);


        }catch(err){}
    }
}



function launch_rollout(back_channel,rollout){
    function two_digits(n){
        if (n < 10) return "0"+n.toString();
        else return n.toString();
    }
    
    rollout.up_since = Date.now();
    rollout.state = ROLLOUT_STARTED_PENDING;
    let t = new Date(rollout.up_since);
    let timestamp = `${t.getFullYear()}-${two_digits(t.getMonth()+1)}-${two_digits(t.getDate())} ${two_digits(t.getHours())}:${two_digits(t.getMinutes())}:${two_digits(t.getSeconds())}`;
    //console.log(`TIMESTAMP: ${timestamp}`);
    log_info("launch_rollout()",`Trigger ${rollout.name}`);

    let ws_api_port = get_next_ws_api_port();
    let rollout_path_suffix = encodeURIComponent(rollout.name)  .replace(/%/g,"_")
                                                                .replace(/\(/g,"LL")
                                                                .replace(/\)/g,"RR")
                                                                .replace(/\$/g,"_S_")
                                                                .replace(/{/g,"LLL")
                                                                .replace(/}/g,"RRR")
                                                                .replace(/\./g,"_DOT_");
    let rollout_path_base = rollout_path_suffix;
    let rollout_run_path = "runs/"+rollout_path_suffix;
    let rollout_db_full = "db_rollout_dump.ceps";
    let rollout_db = rollout_db_full;

    if (global_conf.rollouts != undefined){
        rollout_db_full = global_conf.rollouts;
        rollout_db = path.basename(rollout_db_full);
    } 

    try{fs.mkdirSync(rollout_run_path);}catch(err){}

    try{fs.writeFileSync(rollout_run_path+"/globals.ceps",
    `
    // ${t.toUTCString()}
    // Generated by rollAut
    // DO NOT CHANGE
    val START_TIMESTAMP  = "${timestamp}";
    `);}catch(err){}

    try{fs.writeFileSync(rollout_run_path+"/extract_rollout.ceps",
    `
    // ${t.toUTCString()}
    // Generated by mms_rollout_automation@cepS
    // DO NOT CHANGE

    val rollout_id = "${rollout.name}";

    static_for(e:root.rollout_){
        if (text(strip(e.name.content())) == rollout_id ){
            rollout{
                e.name;
                e.steps;
                markets{
                    static_for(f:e.markets.market_details){
                        market{f.content();};                        
                    }
                };
            };
        }
    }
    `);}catch(err){
        try{if(err != undefined) log_err(`launch_rollout()`,err.stack);}catch(e){} 
    }


    try{
    fs.writeFileSync(rollout_run_path+"/start.sh",
    `    
     cd runs
     cp ../${rollout_db_full} ./${rollout_path_base}
     cd ${rollout_path_base}
     ../../ceps ${rollout_db} \\
             globals.ceps\\
             ../../db_descr/gen.ceps \\
             extract_rollout.ceps \\
             ../../lib/conf.ceps \\
             ../../lib/rollout_step.ceps \\
             ../../transformations/rollout2worker.ceps \\
             ../../transformations/rollout2sm.ceps \\
             ../../transformations/driver4rollout_start_immediately.ceps \\
             --dot_gen_one_file_per_top_level_statemachine \\
             --dot_gen \\
             --ignore_simulations
     mkdir ../../web/${rollout_path_base}__svgs -p
     for e in *.dot ; do
        dot $e -Tsvg -o ../../web/${rollout_path_base}__svgs/$\{e%%.dot\}.svg
     done

     for e in *.dot ; do
        rm $e
     done             
     
    `);}catch(err){
        try{if(err != undefined) log_err(`launch_rollout()`,err.stack);}catch(e){} 
    }

    log_debug("launch_rollout()",`${rollout_run_path}/start.sh Written`);
    let svg_generating_proc = spawn("sh",["runs/"+rollout_path_suffix+"/start.sh"]);
    log_debug("launch_rollout()",`SVG generating process spawned, pid=${svg_generating_proc.pid}`);
    
    svg_generating_proc.stdout.on('data', (data) => {
        log_debug(`launch_rollout()/svg_generating_proc.pid{svg_generating_proc.pid}`,`${data}`);
    });
    svg_generating_proc.stderr.on('data', (data) => {
        log_err(`launch_rollout()/svg_generating_proc.pid{svg_generating_proc.pid}`,`${data}`);
    });
    svg_generating_proc.on('close', (code) => {
        log_debug(`launch_rollout()/svg_generating_proc.pid{svg_generating_proc.pid}`,`Child Process pid=${svg_generating_proc.pid} exited with code ${code}`);
        start_ceps_instance(rollout,rollout_path_base,rollout_db_full,ws_api_port,rollout_path_suffix);
    });
    svg_generating_proc.on('error', (err) => {
        try{if(err != undefined) log_err(`launch_rollout()/svg_generating_proc.pid{svg_generating_proc.pid}`,err.stack);}catch(e){}
    });    
}

function kill_rollout(back_channel,rollout){
    process.kill(rollout.pid);
    rollout.state = ROLLOUT_READY;
    rollout.pid = undefined;
    rollout.ws_api = undefined;
    back_channel.send(JSON.stringify({reply:"ok",rollout:rollout}));
}


function fetch_rollout_plan(callback){
    log_debug(`fetch_rollout_plan()`,"");
    if (global_conf.skip_db_fetch){
        log_debug(`fetch_rollout_plan()`,`skipped (configuration option skip_db_fetch==true)`);
        callback();
        return;
    }
    try{
        p = spawn("./fetch_rollout_plan");
    } catch (e){
     log_err("fetch_rollout_plan",e.stack);
     callback();
     return;   
    }
    log_debug(`fetch_rollout_plan()`,`Spawned Child Process pid=${p.pid}`);

    p.stdout.on('data', (data) => {
        log_debug(`fetch_rollout_plan()`,`${data}`);
    });
    p.stderr.on('data', (data) => {
        log_debug(`fetch_rollout_plan()`,`${data}`);
    });
    p.on('close', (code) => {
        log_debug(`fetch_rollout_plan()`,`Child Process pid=${p.pid} exited`);
        callback();
    });
    p.on('error', (err) => {
        try{if(err != undefined) log_err("fetch_rollout_plan",err.stack);}catch(e){}
    });    
}

console.log(chalk.bold.green(
`Media Markt Saturn Rollout Automation (RollAut) Service, powered by cepS (\"https://github.com/cepsdev/ceps.git\").`));

function main(){

    setInterval(() => {
        log_debug(`main()`,`Check for DB changes`);

        fetch_rollout_plan(
            function(){
                try{
                  fetch_planned_rollouts(undefined,(err)=>{
                      
                  });
                } catch(err){
                    log_err("fetch_rollout_plan",err.stack);
                }
              }
          );
    },
    DB_CHECK_INTERVAL_MS);

    http_port = 3000;
    log_debug(`HTTP Server`,`listening at ${host_name}:${http_port}`);
    http.createServer(app).listen(http_port);

    var start_reading_cmds = function() {
        log_debug(`HTTP Server`,`Ready to receive commands via ws://${host_name}:${command_port}`);    
        const ws_command = new WebSocket.Server({port: command_port});
        ws_command.on("connection", function (ws){
            connections.push(ws);
            ws.on("error",function (err) {
                try{
                 log_err("Receiving Command",err.stack);
                } catch (errr) {
                }
            });
            ws.on("close",function () {
                log_debug("Receiving Command",`Client closed connect.`);
                for(let i = 0; i != connections.length;++i)
                {
                    if (ws == connections[i]){
                        connections.splice(i,1);
                        break;
                    }
                }
            });
            ws.on("message",function incoming(message){
             try{
                log_debug("Receiving Command",message);
                let msg = JSON.parse(message);
                if (msg.cmd == "get_planned_rollouts"){
                    fetch_planned_rollouts(ws,(err)=>{});
                } else if (msg.cmd == "launch_rollout"){                    
                    launch_rollout(ws,save_rollout(msg.rollout));
                } else if (msg.cmd == "kill_rollout"){
                    kill_rollout(ws,lookup_rollout(msg.rollout));
                } else if (msg.cmd == "restart"){
                    restart_sm(ws,msg.what,lookup_rollout(msg.rollout_name));
                } else if (msg.cmd == "kill"){
                    kill_sm(ws,msg.what,lookup_rollout(msg.rollout_name));
                }
            } catch(err){
                log_err("Receiving Command",err.stack);
            }
            });
        });
        ws_command.on("error", function (err){
            log_err("Receiving Command",err != undefined ? err.stack : "NA");             
        });
    };

    setTimeout(start_reading_cmds,10000);

    let check_rollouts = setInterval(
        ()=>{
            
            for(let i = 0; i != rollouts.length;++i){
                let rollout = rollouts[i];
                //console.log(rollout);
                if (rollout.state != ROLLOUT_READY) continue;
                let t = new Date(rollout.when)-Date.now();
                if (t < 1000 && t > -10000){
                    rollout.state = ROLLOUT_STARTED_PENDING;
                    setTimeout( 
                        () => {
                            console.log(`Launch Rollout: '${rollout.name}'`)
                            try{
                                launch_rollout(undefined,rollout);
                            }catch(err){
                                log_err(`check_rollouts`,err.stack);
                            }
                        },
                        Math.min(1000,Math.abs(t))
                    );
                } else if (t < -10000){
                    //rollout.state = ROLLOUT_DONE;
                    //broadcast(JSON.stringify({reply:"ok",rollout:rollout}));
                }
            }        
        },
        1000
    );
    

    setInterval(()=>{
        //fetch_rollout_plan(()=>{console.log(chalk.yellow(`Rollout Plan Updated. `));});        
    },60*60000);
}

fetch_rollout_plan(
  function(){
        fetch_planned_rollouts(undefined,(err)=>{
            if(err) process.exit(1);
            main();
        });
    }
);








