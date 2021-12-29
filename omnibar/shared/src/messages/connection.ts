import { constructWhistMessage } from "../utils/messages";

const clientToServerDiscovery = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "SEND_CLIENT_SERVER_DISCOVERY",
  });

const stopClientServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "STOP_CLIENT_SERVER_CONNECTION",
  });


const serverToClientAck = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "SEND_SERVER_CLIENT_ACK",
  });

export {
  clientToServerDiscovery,
  stopClientServerConnection,
  serverToClientAck
};
