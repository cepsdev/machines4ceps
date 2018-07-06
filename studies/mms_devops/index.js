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
const command_port = 1064;
const ceps_cmd_ = "./ceps";
const fetch_rollouts_ceps_api_port = 10000;
const ws_api_base = 11000;
let ws_api_next = 0;
const ws_api_max = 1000;

const ROLLOUT_READY = 0;
const ROLLOUT_STARTED_PENDING = 1;
const ROLLOUT_STARTED = 2;
const ROLLOUT_STOP_PENDING = 3;


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
 for(let i = 0; i != connections.length;++i){
  if (connections[i].readyState != connections[i].OPEN) continue;
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
        if (rollout.name != rollouts[i].name) continue;
        rollouts[i] = rollout;
     }
    }
}

function lookup_rollout(rollout){
    for(let i = 0; i != rollouts.length;++i){
        if (rollout.name == rollouts[i].name) return rollouts[i];
    }
    return rollout;
}

function get_rollout_by_name(rollout_name){
    for(let i = 0; i != rollouts.length;++i){
        if (rollout_name == rollouts[i].name) return rollouts[i];
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
            if (v[i].name == "when") dest.when = v[i].content[0];
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

app.get("/rollout_status", function(req, res) {
    let rollout_name = req.query.rollout;
    let rollout = get_rollout_by_name(rollout_name);
    if (rollout == undefined) {
        rollout={name:rollout_name};
        res.render("404",{ 
            command_port : command_port,rollout:rollout  }
        );
    } else res.render("index",{ 
        hostname_ceps_service: host_name, 
        ceps_api_port : rollout.ws_api,
        server_name : host_name,
        command_port : command_port,
        rollout      : rollout    
     }
    );
});

app.get("/", function(req, res) {
    res.render("rollout_admin",{ 
        server_name : host_name,
        command_port : command_port  }
    );
});

let get_planned_rollouts = undefined;

let fetch_planned_rollouts = function (back_channel){

    if (rollouts != undefined && rollouts.length>0){
        back_channel.send(JSON.stringify(rollouts));
        return ;
    }
    let params = ceps_cmd_add_param_ws_api([`db_rollout_dump.ceps`,`db_descr/gen.ceps`,`drivers/empty_simulation.ceps`],
    `${fetch_rollouts_ceps_api_port}`);
    ceps_process = spawn(ceps_cmd(),params);                        
    console.log(chalk.yellow(`fetch_planned_rollouts(): Spawned Child Process pid=${ceps_process.pid}\nCommand: ${ceps_cmd()} ${params.join(" ")}`));
    let proc_down = false;
    ceps_process.stdout.on('data', (data) => {
        console.log(chalk.yellow(`${data}`));
    });
    ceps_process.stderr.on('data', (data) => {
        console.log(chalk.yellow(`${data}`));
    });
    ceps_process.on('close', (code) => {
        proc_down = true;
        console.log(chalk.yellow(`fetch_planned_rollouts(): Child Process pid=${ceps_process.pid} exited with code ${code}`));
    });

    let fetch_proc = setInterval(()=>{
        let ws_ceps_api = new WebSocket(`ws://localhost:${fetch_rollouts_ceps_api_port}`);
        ws_ceps_api.on("error", () => {console.log(chalk.red(`***Error: Connect to ws://localhost:${fetch_rollouts_ceps_api_port} failed`));} );
        ws_ceps_api.on("open", () => {
            ws_ceps_api.on("message", function (msg){
                console.log(msg);

                let m = JSON.parse(msg).sresult;
                rollouts = extract_rollout_data_from_webservice_result(m);
                console.log(chalk.yellow(`fetch_planned_rollouts(): ${rollouts}`));
                try{back_channel.send(JSON.stringify(rollouts));}catch(err){}
                clearInterval(fetch_proc);
                process.kill(ceps_process.pid);
            });
            ws_ceps_api.send("QUERY root.rollout_;");         
        } );
        ws_ceps_api.on("close", () => {} );
       },1000);  
}

function launch_rollout(back_channel,rollout){
    function two_digits(n){
        if (n < 10) return "0"+n.toString();
        else return n.toString();
    }
    rollout.up_since = Date.now();
    let t = new Date(rollout.up_since);
    let timestamp = `${t.getFullYear()}-${two_digits(t.getMonth()+1)}-${two_digits(t.getDay()+1)} ${two_digits(t.getHours())}:${two_digits(t.getMinutes())}:${two_digits(t.getSeconds())}`;

    console.log(`launch_rollout() - ${timestamp}`);
    
    let ws_api_port = get_next_ws_api_port();
    let rollout_path_suffix = encodeURIComponent(rollout.name).replace("%","_");
    let rollout_path_base = rollout_path_suffix;
    let rollout_run_path = "runs/"+rollout_path_suffix;



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
     cp ../db_rollout_dump.ceps ./${rollout_path_base}
     cd ${rollout_path_base}
     ../../ceps db_rollout_dump.ceps \\
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

    spawn("sh",["runs/"+rollout_path_suffix+"/start.sh"]);

    process.chdir(`runs`);

    let cmd_args = [];

    ceps_process = spawn("../ceps",
        cmd_args = [
            `${rollout_path_base}/globals.ceps`,
            `${rollout_path_base}/db_rollout_dump.ceps`,
            "../db_descr/gen.ceps",
            `${rollout_path_base}/extract_rollout.ceps`,
            "../lib/conf.ceps",
            "../lib/rollout_step.ceps",
            "../transformations/rollout2worker.ceps",
            "../transformations/rollout2sm.ceps",
            "../transformations/driver4rollout_start_immediately.ceps",
            "--dot_gen_one_file_per_top_level_statemachine",
            "--dot_gen",
            "--no_file_output",
            "--ws_api",`${ws_api_port}`
        ]
    );

    console.log(chalk.yellow(`launch_rollout() executes cmd: ../ceps ${cmd_args.join(" ")}`));


    process.chdir(`..`);

    console.log(chalk.yellow(`launch_rollout(): Spawned Child Process pid=${ceps_process.pid}`));

    let proc_down = false;
    ceps_process.stdout.on('data', (data) => {
        console.log(chalk.yellow(`${data}`));
    });
    ceps_process.stderr.on('data', (data) => {
        console.log(chalk.yellow(`${data}`));
    });
    ceps_process.on('close', (code) => {
        proc_down = true;
        connections_with_ceps_cores[rollout.pid.toString] = undefined;
        console.log(chalk.yellow(`launch_rollout(): Child Process pid=${ceps_process.pid} exited with code ${code}`));
    });

    let fetch_proc = setInterval(()=>{
        let ws_ceps_api = new WebSocket(`ws://localhost:${ws_api_port}`);
        ws_ceps_api.on("error", () => {console.log(chalk.red(`***Error: Connect to ws://localhost:${ws_api_port} failed`));} );
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
                        ws_ceps_api.send("QUERY root.__proc.coverage");
                    },10000
                );
                ws_ceps_api.on("message", function(msg){
                    //console.log(msg);
                    let reply_ = JSON.parse(msg);
                    if (!reply_.ok) return;
                    let reply = {};
                    simplifyceps2json(reply,reply_.sresult.coverage);
                    reply = reply.coverage[0];
                    //console.log(reply);
                    rollout.coverage = reply.transition_coverage[0].ratio;
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

fetch_rollout_plan(
  function(){
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
                        fetch_planned_rollouts(ws);
                    } else if (msg.cmd == "launch_rollout"){
                        launch_rollout(ws,msg.rollout);
                    } else if (msg.cmd == "kill_rollout"){
                        kill_rollout(ws,lookup_rollout(msg.rollout));
                    }
                });
            });
        };

        setTimeout(start_reading_cmds,5000);

        setInterval(()=>{
            //fetch_rollout_plan(()=>{console.log(chalk.yellow(`Rollout Plan Updated. `));});        
        },60*60000);
    }
);









