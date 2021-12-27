import { constructWhistMessage } from "../utils/messages";

const initializeClientServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "INITIALIZE_CLIENT_SERVER_CONNECTION",
    from: "client",
    to: "server",
  });

const stopClientServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "STOP_CLIENT_SERVER_CONNECTION",
    from: "client",
    to: "server",
  });

const initializeBrowserServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "INITIALIZE_BROWSER_SERVER_CONNECTION",
    from: "browser",
    to: "server",
  });

const stopBrowserServerConnection = (sessionID: number) =>
  constructWhistMessage(sessionID, {
    name: "STOP_BROWSER_SERVER_CONNECTION",
    from: "browser",
    to: "server",
  });

export {
  initializeClientServerConnection,
  stopClientServerConnection,
  initializeBrowserServerConnection,
  stopBrowserServerConnection,
};
