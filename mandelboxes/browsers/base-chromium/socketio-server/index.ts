import express from "express"
import http from "http"
import { Server as Socketio } from "socket.io"

const expressServer = express()
const server = http.createServer(expressServer)
const io = new Socketio(server)

io.on("connection", () => {
    console.log("Socket connected!")
})

server.listen(32261)