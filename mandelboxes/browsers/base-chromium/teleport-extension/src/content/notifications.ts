import {
    ContentScriptMessage,
    ContentScriptMessageType,
} from "@app/constants/ipc"
import { injectResourceIntoDOM } from "@app/utils/dom"

const initNotificationSender = () => {
    injectResourceIntoDOM(document, "js/notifications.js")

    // TODO: listen for client-side interactions with the notification and send them back?
}

export { initNotificationSender }
