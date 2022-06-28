import { io } from "socket.io-client"

const initSocketioConnection = () => {
    console.log("trying to connect")
    io(`http://localhost:32261`, {
        reconnectionDelayMax: 10000,
    })
}

export { initSocketioConnection }