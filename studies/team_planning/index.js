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
const global_conf = require('./rollaut.json');

function main(){
    spawn("./staccato");
    let app = express();
    app.get("/", function(req, res) {
        res.render("index",{ 
            server_name : host_name
             }
        );
    });
    app.set('trust proxy', true);
    let publicPath = path.resolve(__dirname, "web");
    app.set("views", path.resolve(__dirname, "web/views"));
    app.set("view engine", "ejs");
    app.use(express.static(publicPath));
    http_port = 3000;
    http.createServer(app).listen(http_port);
}

main();









