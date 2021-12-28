import { constructWhistMessage } from "../utils/messages";

const initializeClientServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "INITIALIZE_CLIENT_SERVER_CONNECTION",
  });

const stopClientServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "STOP_CLIENT_SERVER_CONNECTION",
  });

const initializeBrowserServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "INITIALIZE_BROWSER_SERVER_CONNECTION",
  });

const stopBrowserServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "STOP_BROWSER_SERVER_CONNECTION",
  });

export {
  initializeClientServerConnection,
  stopClientServerConnection,
  initializeBrowserServerConnection,
  stopBrowserServerConnection,
};
