import { socket } from "@app/utils/socketio"
import { initializeBrowserServerConnection } from "@whist/shared"

socket.emit(initializeBrowserServerConnection(1234).name)
