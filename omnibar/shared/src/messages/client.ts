import { constructWhistMessage } from "../utils/messages";

const sendDesiredUrls = (sessionID: number, urls: Array<{ url: string, id: number }>) =>
  constructWhistMessage(sessionID, {
    name: "SEND_DESIRED_URLS",
    body: { urls }
  });

const openDesiredUrl = (sessionID: number, id: number) =>
  constructWhistMessage(sessionID, {
    name: "OPEN_DESIRED_URL",
    body: { id }
  });

export { sendDesiredUrls, openDesiredUrl }