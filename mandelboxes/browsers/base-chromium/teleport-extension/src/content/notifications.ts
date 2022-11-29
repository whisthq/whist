import {
    ContentScriptMessage,
    ContentScriptMessageType,
} from "@app/constants/ipc"
import { injectResourceIntoDOM } from "@app/utils/dom"

const initNotificationSender = () => {
    injectResourceIntoDOM(document, "js/notifications.js")

    const notificationCallback = (
        title: string,
        opt: NotificationOptions | undefined,
    ) => {
        console.log("Notification callback called", title, opt);
        chrome.runtime.sendMessage(<ContentScriptMessage>{
            type: ContentScriptMessageType.SERVER_NOTIFICATION,
            value: {
                title: title,
                opt: opt
            }
        });
    }

    // Intercept server-side notifications and send them to the client
    // currently copied from https://github.com/nativefier/nativefier/blob/master/app/src/preload.ts
    // TODO: also send over permissions? is this needed? or the client has to communicate permissions
    const setNotificationCallback = (
        // createCallback: ((title: string, opt: NotificationOptions | undefined) => void)
        createCallback: any
    ) => {
        console.log("setNotificationCallback called!");
        const oldNotify = window.Notification;
        console.log("oldNotify", oldNotify);
        const newNotify = new Proxy(Notification, {
            construct : (target: any, args: any[]) => {
                createCallback(args);
                return new target(args);
            }
        });
        console.log("newNotify", newNotify);

        window.Notification = newNotify;
        console.log("supposedly set window.Notification", window.Notification);
    }
    setNotificationCallback(notificationCallback);

    // TODO: listen for client-side interactions with the notification and send them back?
}

export { initNotificationSender }
