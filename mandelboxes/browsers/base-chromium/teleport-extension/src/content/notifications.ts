import {
    ContentScriptMessage,
    ContentScriptMessageType,
} from "@app/constants/ipc"
import { injectResourceIntoDOM } from "@app/utils/dom"

const initNotificationSender = () => {
    injectResourceIntoDOM(document, "js/notifications.js")

    // Intercept server-side notifications and send them to the client
    // currently copied from https://github.com/nativefier/nativefier/blob/master/app/src/preload.ts
    // TODO: also send over permissions? is this needed? or the client has to communicate permissions
    const setNotificationCallback = (
        createCallback: ((title: string, opt: NotificationOptions | undefined) => void)
    ) => {
        console.log("setNotificationCallback called!");
        const oldNotify = window.Notification;
        class newNotify extends Notification {
            constructor(title: string, options?: NotificationOptions | undefined) {
                console.log("window notification created", title, options);
                createCallback(title, options);
                super(title, options);
                // return this;
            }
        }

        window.Notification = newNotify;
    }

    const notificationCallback = (
        title: string,
        opt: NotificationOptions | undefined,
    ) => {
        console.log("Notification callback called");
        console.log(title, opt);
        chrome.runtime.sendMessage(<ContentScriptMessage>{
            type: ContentScriptMessageType.SERVER_NOTIFICATION,
            value: {
                title: title,
                opt: opt
            },
        });
    };
    setNotificationCallback(notificationCallback);

    // TODO: listen for client-side interactions with the notification and send them back?
}

export { initNotificationSender }
