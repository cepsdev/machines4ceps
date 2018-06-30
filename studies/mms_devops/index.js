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


let app = express();
app.set('trust proxy', true);
let publicPath = path.resolve(__dirname, "web");
app.set("views", path.resolve(__dirname, "web/views"));
app.set("view engine", "ejs");
app.use(express.static(publicPath));

app.get("/", function(req, res) {
    
    res.render("index",{ 
        hostname_ceps_service: host_name, 
        ceps_api_port : 1063,
        server_name : host_name,
        command_port : command_port  }
    );

});




const ws_command = new WebSocket.Server({port: command_port});

ws_command.on("connection", function connection(ws){
    let ceps_service_running = false;
    let ceps_process = undefined;

    function start_ceps_service() {
        if(ceps_service_running) {console.log("cepS service already running.");return;}
        ceps_service_running = true;
        console.log(JSON.stringify({event:"ceps service started"}));
        ws.send(JSON.stringify({event:"ceps service started"}));

        ceps_process = spawn("sh",["start.sh"]);
        ceps_process.stdout.on('data', (data) => {
            console.log(chalk.yellow(`${data}`));
        });
        ceps_process.stderr.on('data', (data) => {
            console.log(chalk.yellow(`${data}`));
        });
        ceps_process.on('close', (code) => {
            ws.send(JSON.stringify({event:"ceps service stopped"}));
                    ceps_service_running = false;
            console.log(chalk.yellow("start.sh exited with code "+`${code}`));
        });
    }

    function stop_ceps_service(){
        if(!ceps_service_running) return;
        if (ceps_process == undefined) return;
        ceps_process.kill();ceps_process = undefined;
    }

    ws.on("message",function incoming(message){
        let msg = JSON.parse(message);
        console.log(msg);
        if (msg.cmd == "start"){
            start_ceps_service();    
        } else if (msg.cmd == "stop"){
            stop_ceps_service();    
        }

    });
});

http.createServer(app).listen(3000);

///////////////////////////////////////////////////////////////
////////////////// LOGIC
///////////////////////////////////////////////////////////////








