import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { injectResourceIntoDOM } from "@app/utils/dom"

const initNotificationHandler = () => {
    injectResourceIntoDOM(document, "js/notifications.js")

    // Intercept server-side notifications and send them to the client
    // currently copied from https://github.com/nativefier/nativefier/blob/master/app/src/preload.ts
    function setNotificationCallback(
        createCallback: {
            (title: string, opt: NotificationOptions): void;
        },
    ): void {
        const oldNotify = window.Notification;
        const newNotify = function (
            title: string,
            opt: NotificationOptions,
        ): Notification {
            createCallback(title, opt);
            return new oldNotify(title, opt);
        };
        newNotify.requestPermission = oldNotify.requestPermission.bind(oldNotify);
        Object.defineProperty(newNotify, "permission", {
            get: () => oldNotify.permission,
        });
        window.Notification = newNotify;
    }

    function notificationCallback(
        title: string,
        opt: NotificationOptions,
    ): void {
        console.log({title, opt});
        chrome.runTime.sendMessage(<ContentScriptMessage>{
            type: ContentScriptMessageType.SERVER_NOTIFICATION,
            value: {
                title: title,
                opt: opt
            },
        })
    }
    setNotificationCallback(notificationCallback);

    // TODO: listen for client-side interactions with the notification and send them back?
}


