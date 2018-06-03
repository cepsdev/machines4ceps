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


let app = express();
app.set('trust proxy', true);
let publicPath = path.resolve(__dirname, "web");

app.use(express.static(publicPath));
http.createServer(app).listen(3000);
