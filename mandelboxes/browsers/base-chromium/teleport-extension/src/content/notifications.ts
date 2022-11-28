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
    function setNotificationCallback(
        createCallback: {
            (title: string, opt?: NotificationOptions | undefined): void;
        },
    ): void {
        class newNotify extends Notification {
            constructor(title: string, options?: NotificationOptions | undefined) {
                super(title, options);
                createCallback(title, options);
            }
        }

        /*
        const newNotify = function (
            title: string,
            opt: NotificationOptions,
        ): Notification {
            createCallback(title, opt);
            return new oldNotify(title, opt);
        };
        newNotify.requestPermission = oldNotify.requestPermission.bind(oldNotify);
        Object.defineProperty(newNotify.prototype, 'permission', {
            get: () => oldNotify.permission,
        });
        */
        window.Notification = newNotify;
    }

    function notificationCallback(
        title: string,
        opt?: NotificationOptions | undefined,
    ): void {
        console.log({title, opt});
        chrome.runtime.sendMessage(<ContentScriptMessage>{
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

export { initNotificationSender }
