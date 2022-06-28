import express from "express"
import http from "http"
import { Server as Socketio, Socket } from "socket.io"


// Initialize socket.io server
const expressServer = express()
const server = http.createServer(expressServer)
const io = new Socketio(server, {
  cors: {
    origin: "*",
  },
})

// Listens for client/server events and broadcasts them to the other side
io.on("connection", (socket: Socket) => {
  if (io.engine.clientCount === 2) io.emit("connected")
  socket.onAny((eventName, ...args) => {
    socket.broadcast.emit(eventName, args)
  })
})

// Listens for connections on port 32261
server.listen(32261)
