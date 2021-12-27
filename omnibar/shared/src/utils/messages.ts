import { WhistMessage } from "../types/messages"

const constructWhistMessage = (sessionID: number, message: Pick<WhistMessage, Exclude<keyof WhistMessage, "sessionID">>):WhistMessage => ({
    sessionID,
    ...message
})

export { constructWhistMessage }