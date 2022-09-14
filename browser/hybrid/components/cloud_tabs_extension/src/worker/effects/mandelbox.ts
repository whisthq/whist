import { zip, merge } from "rxjs"
import { withLatestFrom } from "rxjs/operators"

import {
  mandelboxError,
  mandelboxNeeded,
  mandelboxSuccess,
} from "@app/worker/events/mandelbox"
import { webuiMandelboxNeeded } from "@app/worker/events/webui"
import { hostSuccess, hostError } from "@app/worker/events/host"
import { socket, socketReconnectFailed } from "@app/worker/events/socketio"

import { serializeProtocolArgs } from "@app/worker/utils/protocol"
import { addTabToQueue } from "@app/worker/utils/tabs"
import { whistState } from "@app/worker/utils/state"

import { MandelboxState } from "@app/constants/storage"
import { Socket } from "socket.io-client"

mandelboxNeeded.subscribe(() => {
  whistState.mandelboxState = MandelboxState.MANDELBOX_WAITING
})

merge(mandelboxNeeded, socketReconnectFailed)
  .pipe(withLatestFrom(socket))
  .subscribe(([, socket]: [any, Socket]) => {
    socket.close()
  })

zip(mandelboxSuccess, hostSuccess).subscribe(([mandelbox, host]: any) => {
  const s = serializeProtocolArgs({ ...mandelbox, ...host })
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "MANDELBOX_INFO",
      value: {
        "server-ip": s.mandelboxIP,
        p: s.mandelboxPorts,
        k: s.mandelboxSecret,
      },
    })
  )

  whistState.mandelboxState = MandelboxState.MANDELBOX_CONNECTED
  whistState.mandelboxInfo = { ...mandelbox, ...host }
})

webuiMandelboxNeeded.subscribe(() => {
  if (whistState.mandelboxState !== MandelboxState.MANDELBOX_WAITING) {
    whistState.mandelboxState = MandelboxState.MANDELBOX_NONEXISTENT
  }
  whistState.mandelboxInfo = undefined
})

merge(mandelboxError, hostError, socketReconnectFailed).subscribe(() => {
  whistState.mandelboxState = MandelboxState.MANDELBOX_NONEXISTENT
  whistState.mandelboxInfo = undefined
})

merge(webuiMandelboxNeeded, socketReconnectFailed).subscribe(() => {
  whistState.activeCloudTabs = []

  chrome.tabs.query({}, (tabs: chrome.tabs.Tab[]) => {
    const cloudTabs = tabs.filter((tab) => tab.url?.startsWith("cloud:"))

    cloudTabs.forEach((tab) => {
      addTabToQueue(tab)
    })
  })
})
