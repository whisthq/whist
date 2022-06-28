import { io } from "socket.io-client"

const initSocketioConnection = () => {
    io(`http://localhost:32261`, {
        reconnectionDelayMax: 10000,
    })
}

export { initSocketioConnection }