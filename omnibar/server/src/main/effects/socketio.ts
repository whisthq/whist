import { app } from "electron"
import express from "express"
import http from "http"
import { Server as Socketio } from "socket.io"

import { MANDELBOX_PORT, LOCAL_PORT } from "@app/constants/networking"
import { clientToServerDiscovery } from "@whist/shared"

const expressServer = express()
const server = http.createServer(expressServer)
const io = new Socketio(server)

// Whenever any socketio-client connects, this function will fire
io.on("connection", (socket) => {
  socket.on(clientToServerDiscovery(1234).name, () => {
    console.log("Received client!")
  })
})

// Tells express to listen on port 3000 (local) or
// port 32261, which is the free TCP port the host service has
// assigned to use within a mandelbox
server.listen(app.isPackaged ? MANDELBOX_PORT : LOCAL_PORT)
