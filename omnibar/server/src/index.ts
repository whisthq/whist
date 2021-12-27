import express from "express";
import http from "http";
import { Server as Socketio } from "socket.io";

const app = express();
const server = http.createServer(app);
const io = new Socketio(server);

// Whenever any socketio-client connects, this function will fire
io.on("connection", (socket) => {
  socket.on("Client connected", (msg) => {
    console.log("Received client", msg);
  });
});

// Tells express to listen on port 3000 (local) or
// process.env.PORT (if deployed on Heroku)
server.listen(process.env.PORT || 3000);
