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

const global_conf = require('./rollaut.json');

console.log(global_conf);
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
 for(let i = 0; i != connections.length;++i){
  if (connections[i].readyState != connections[i].OPEN) continue;
  //if(!b){console.log(`broadcasting: ${msg}`);console.log();b=true;}
  connections[i].send(msg);
 }
}

let rollouts = [
    /*
    {id:"1234",state::ROLLOUT_...,when:"28-03-2019 00:00:00",steps:[{name:"abc"},{name:"def"}],markets:[{name:"m1",sap_code:"123"}]}
    */
];

function save_rollout(rollout){
    if (rollouts.length == 0)
     rollouts.push(rollout);
    else {
     for(let i = 0; i != rollouts.length;++i){
        if (rollout.id != rollouts[i].id) continue;
        rollouts[i] = rollout;
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
    console.log(`CHANGED = ${changed}`);

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


    ceps_process = spawn(ceps_cmd(),params);                        
    log_debug(`fetch_planned_rollouts()`,`Spawned Child Process pid=${ceps_process.pid}\nCommand: ${ceps_cmd()} ${params.join(" ")}`);
    let proc_down = false;
    ceps_process.stdout.on('data', (data) => {
        //console.log(chalk.yellow(`${data}`));
    });
    ceps_process.stderr.on('data', (data) => {
        log_err(`fetch_planned_rollouts()/cepS core pid=${ceps_process.pid}`,`${data}`);
    });
    ceps_process.on('close', (code) => {
        proc_down = true;
        try{process.kill(ceps_process.pid);}catch(err){}
        log_debug(`fetch_planned_rollouts()/cepS core pid=${ceps_process.pid}`,`exited with code ${code}`);
    });

    let fetch_proc = function () {
        //let current_ws_api_port = fetch_rollouts_ceps_api_port;
        log_debug(`fetch_planned_rollouts()/fetch_proc()`,`Trying to establish a websocket connect to localhost:${current_ws_api_port} pid=${ceps_process.pid}`);
        let ws_ceps_api = new WebSocket(`ws://localhost:${current_ws_api_port}`);
        ws_ceps_api.on("error", (err) => {
            console.log(chalk.red(`***Error: Connect to ws://localhost:${current_ws_api_port} failed pid=${ceps_process.pid}`));
            if (!proc_down) setTimeout(fetch_proc,1000);
            else callback(err);
        });
        ws_ceps_api.on("open", () => {
            ws_ceps_api.on("message", function (msg){
                log_debug(`fetch_planned_rollouts()/fetch_proc()`,`Received a reply from localhost:${current_ws_api_port} pid=${ceps_process.pid}`);
                let m = JSON.parse(msg).sresult;
                let removed_rollouts = [];
                new_rollouts = extract_rollout_data_from_webservice_result(m);
                let changed = merge_rollouts(new_rollouts,removed_rollouts);
                if (changed){
                    log_debug(`fetch_planned_rollouts()/db update`,`Changes in DB detected`);
                    for(let i = 0; i != rollouts.length;++i){
                    let rollout = rollouts[i];
                     if (rollout.changed != undefined && rollout.changed){
                        log_debug(`fetch_planned_rollouts()/db update`,`Rollout id=${rollout.id} changed, trigger broadcast. pid=${ceps_process.pid}`);
                      broadcast(JSON.stringify({reply:"ok",rollout:rollout}));
                     }
                 }
                 for(let i = 0; i != removed_rollouts.length;++i){
                    let rollout = removed_rollouts[i];
                    let r = {id:rollout.id};
                    console.log("Removing Rollout with id = "+r.id);
                    broadcast(JSON.stringify({reply:"ok",rollout:r}));
                 }
                } else console.log("merge_rollouts: no changes detected");
                try{if (back_channel != undefined) back_channel.send(JSON.stringify(rollouts));}catch(err){}
                log_debug(`fetch_planned_rollouts()/fetch_proc()`,`Shutting down of process with pid=${ceps_process.pid}`);
                let kill_proc = ()=> {
                    try{process.kill(ceps_process.pid);}catch(err){
                        log_err("fetch_planned_rollouts()/fetch_proc():kill_proc",`Failed to kill process with pid=${ceps_process.pid}. Retry in 1 second.`);
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
       setTimeout(fetch_proc,2000);
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
    console.log(`${make_timestamp()} ${who} ${msg}`);
}
function log_err(who,msg){
    console.log(chalk.red(`${make_timestamp()} ${who} ${msg}`));
}
function log_debug(who,msg){
    if (global_conf.log_debug != undefined && global_conf.log_debug) console.log(chalk.yellow(`${make_timestamp()} ${who} ${msg}`));
}

function launch_rollout(back_channel,rollout){
    function two_digits(n){
        if (n < 10) return "0"+n.toString();
        else return n.toString();
    }
    rollout.up_since = Date.now();
    let t = new Date(rollout.up_since);
    let timestamp = `${t.getFullYear()}-${two_digits(t.getMonth()+1)}-${two_digits(t.getDate())} ${two_digits(t.getHours())}:${two_digits(t.getMinutes())}:${two_digits(t.getSeconds())}`;
    //console.log(`TIMESTAMP: ${timestamp}`);
    log_info("launch_rollout()",`Trigger ${rollout.name}`);

    let ws_api_port = get_next_ws_api_port();
    let rollout_path_suffix = encodeURIComponent(rollout.name).replace("%","_");
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
    // Generated by mms_rollout_automation@cepS
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
    `);}catch(err){}


    try{fs.writeFileSync(rollout_run_path+"/start.sh",
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
     
    `);}catch(err){}
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
        log_debug(`launch_rollout()/svg_generating_proc.pid{svg_generating_proc.pid}`,`Child Process pid=${ceps_process.pid} exited with code ${code}`);
    });


    process.chdir(`runs`);

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
            "--ws_api",`${ws_api_port}`
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
        "--ws_api",`${ws_api_port}`
    ];

    ceps_process = spawn("../ceps",cmd_args);
    log_debug("launch_rollout()",`Executing ../ceps ${cmd_args.join(" ")}`);
    process.chdir(`..`);
    log_info("launch_rollout()",`Spawned Child Process pid=${ceps_process.pid}`);

    let proc_down = false;
    let ceps_core_err = "";
    ceps_process.stdout.on('data', (data) => {
        log_debug(`launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${data}`);
        ceps_core_err = ceps_core_err+`${data}`;
    });
    ceps_process.stderr.on('data', (data) => {
        log_err(`launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${data}`); 
        ceps_core_err = ceps_core_err+`${data}`;
    });
    ceps_process.on('close', (code) => {
        proc_down = true;
        connections_with_ceps_cores[ceps_process.pid] = undefined;
        log_debug(`launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`exited with code ${code}`);    
    });

    let fetch_proc = setInterval(()=>{
        if (proc_down){
            rollout.state = ROLLOUT_FAILED;
            rollout.error_details = ceps_core_err;
            broadcast(JSON.stringify({reply:"ok",rollout:rollout}));
            clearInterval(fetch_proc);
            log_err(`launch_rollout()/cepS core running ${rollout.name}`,`failed, giving up`); 
            return;
        }
        let ws_ceps_api = new WebSocket(`ws://localhost:${ws_api_port}`);
        ws_ceps_api.on("error", () => {
            log_info(`launch_rollout()/Establishing connection to cepS core running ${rollout.name}`,`ws://localhost:${ws_api_port} not ready yet`);
        } );
        ws_ceps_api.on("open", () => {
            let f = undefined;
            ws_ceps_api.on("message", f = function (msg){
                let m = JSON.parse(msg).sresult;
                clearInterval(fetch_proc);
                rollout.state = ROLLOUT_STARTED;
                rollout.pid = ceps_process.pid;
                rollout.ws_api = ws_api_port;
                rollout.hostname = host_name;
                rollout.url = "/rollout_status?rollout="+encodeURIComponent(rollout.name);
                rollout.svgs = encodeURIComponent(rollout.name).replace("%","_")+"__svgs";
                save_rollout(rollout);
                //back_channel.send(JSON.stringify({reply:"ok",rollout:rollout}));
                broadcast(JSON.stringify({reply:"ok",rollout:rollout}));

                connections_with_ceps_cores[rollout.pid.toString] = ws_ceps_api; 
                ws_ceps_api.removeEventListener("message",f);
                let periodic_status = setInterval(
                    function(){
                        if (proc_down) clearInterval(periodic_status);
                        try{ws_ceps_api.send("QUERY root.__proc.coverage");}catch(err){
                            log_err(`launch_rollout()/cepS core running,pid=${ceps_process.pid} ${rollout.name}`,`${err}`); 
                        }
                    },10000
                );
                ws_ceps_api.on("message", function(msg){
                    let reply_ = JSON.parse(msg);
                    if (!reply_.ok) return;
                    let reply = {};
                    simplifyceps2json(reply,reply_.sresult.coverage);
                    //console.log(reply);

                    //reply = reply.coverage[0];
                    //console.log(reply);
                    rollout.coverage = reply.coverage[0].transition_coverage[0].ratio;
                    rollout.health = reply.categories[0];
                    //console.log(rollout.health);
                    try{broadcast(JSON.stringify({reply:"ok",rollout:rollout}));}catch(err){}
                });
                ws_ceps_api.send("QUERY root.__proc.coverage");
            });
            ws_ceps_api.send("QUERY root.rollout;");         
        } );
        ws_ceps_api.on("close", () => {} );
       },250);
}

function kill_rollout(back_channel,rollout){
    process.kill(rollout.pid);
    rollout.state = ROLLOUT_READY;
    rollout.pid = undefined;
    rollout.ws_api = undefined;
    back_channel.send(JSON.stringify({reply:"ok",rollout:rollout}));
}


function fetch_rollout_plan(callback){
    console.log(chalk.yellow(`fetch_rollout_plan()`));
    if (global_conf.skip_db_fetch){
        console.log(chalk.yellow(`fetch_rollout_plan():skipped (configuration option skip_db_fetch==true)`));
        callback();
        return;
    }
    try{
     p = spawn("./fetch_rollout_plan");
    } catch (e){
     return;   
    }
    console.log(chalk.yellow(`fetch_rollout_plan(): Spawned Child Process pid=${p.pid}`));

    p.stdout.on('data', (data) => {
        console.log(chalk.yellow(`${data}`));
    });
    p.stderr.on('data', (data) => {
        console.log(chalk.yellow(`${data}`));
    });
    p.on('close', (code) => {
        console.log(chalk.yellow(`fetch_rollout_plan(): Child Process pid=${p.pid} exited.`));
        callback();
    });    
}

console.log(chalk.bold.green(`Media Markt Saturn Rollout & Automation Service, powered by cepS (\"https://github.com/cepsdev/ceps.git\").
written by Tomas Prerovsky <tomas.prerovsky@gmail.com>
`));


function main(){

    setInterval(() => {
        log_debug(`main()`,`Check for DB changes`);

        fetch_rollout_plan(
            function(){
                  fetch_planned_rollouts(undefined,(err)=>{
                      
                  });
              }
          );
    },
    15000);

    http_port = 3000;
    console.log(chalk.yellow(`HTTP Server Ready,listening at ${host_name}:${http_port}`));
    http.createServer(app).listen(http_port);

    var start_reading_cmds = function() {
        console.log(chalk.yellow(`Ready to receive commands via ws://${host_name}:${command_port}`));
        const ws_command = new WebSocket.Server({port: command_port});

        ws_command.on("connection", function connection(ws){
            connections.push(ws);
            ws.on("message",function incoming(message){
                let msg = JSON.parse(message);
                console.log(msg);
                if (msg.cmd == "get_planned_rollouts"){
                    fetch_planned_rollouts(ws,(err)=>{});
                } else if (msg.cmd == "launch_rollout"){
                    launch_rollout(ws,msg.rollout);
                } else if (msg.cmd == "kill_rollout"){
                    kill_rollout(ws,lookup_rollout(msg.rollout));
                }
            });
        });
    };

    setTimeout(start_reading_cmds,10000);

    let check_rollouts = setInterval(
        ()=>{
            
            for(let i = 0; i != rollouts.length;++i){
                let rollout = rollouts[i];
                if (rollout.state != ROLLOUT_READY) continue;
                let t = new Date(rollout.when)-Date.now();
                if (t < 1000){
                    rollout.state = ROLLOUT_STARTED_PENDING;
                    setTimeout( 
                        () => {
                            console.log(`Launch Rollout: '${rollout.name}'`)
                            launch_rollout(undefined,rollout);
                        },
                        Math.min(1000,Math.abs(t))
                    );
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








