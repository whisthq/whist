import { socket } from "@app/utils/socketio"
import { initializeBrowserServerConnection } from "@whist/shared"

console.log("emitting message!")
socket.emit(initializeBrowserServerConnection(1234).name)
