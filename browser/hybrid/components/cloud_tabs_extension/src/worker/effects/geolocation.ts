import { socket } from "@app/worker/events/socketio"
import { geolocationRequested } from "@app/worker/events/geolocation"
import { getTab } from "@app/worker/utils/tabs"

import { withLatestFrom } from "rxjs/operators"
import { Socket } from "socket.io-client"

geolocationRequested
  .pipe(withLatestFrom(socket))
  .subscribe(
    ([[requestParams, metaTagName, tabId], socket]: [
      [any, string, number],
      Socket
    ]) => {
      console.log("geolocationRequested")
      ;(chrome as any).whist.broadcastWhistMessage(
        JSON.stringify({
          type: "GEOLOCATION_REQUESTED",
          value: {
            id: tabId,
            metaTagName: metaTagName,
            params: requestParams
          },
        })
      )
    }
  )