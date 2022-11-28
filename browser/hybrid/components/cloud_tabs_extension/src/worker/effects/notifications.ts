import { socket } from "@app/worker/events/socketio"
import { notificationReceived } from "@app/worker/events/notifications"

import { withLatestFrom } from "rxjs/operators"
import { Socket } from "socket.io-client"

// when the socket receives a message from the server, send a whist message (client-internal)
// this will be picked up by stuff in cloud_tabs_ui to actually show the notification
notificationReceived
.pipe(withLatestFrom(socket))
.subscribe(
    ([[title, options], socket]: [
        [string, NotificationOptions | undefined],
        Socket
    ]) => {
        ;(chrome as any).whist.broadcastWhistMessage(
            JSON.stringify({
                type: "SERVER_NOTIFICATION",
                value: {
                    title: title,
                    opt: options
                },
            })
        )
        console.log({"Received notification", title, options});
    }
)
