import { Socket } from "socket.io-client"
import { merge } from "rxjs"
import { withLatestFrom, filter } from "rxjs/operators"
import find from "lodash.find"

import {
  socket,
  socketConnected,
  socketDisconnected,
} from "@app/worker/events/socketio"
import { serverCookiesSynced } from "@app/worker/events/cookies"
import {
  tabActivated,
  tabRemoved,
  tabUpdated,
  tabFocused,
  tabZoomed,
} from "@app/worker/events/tabs"
import { webuiNavigate, webuiRefresh } from "@app/worker/events/webui"
import {
  getActiveTab,
  isCloudTab,
  markActiveCloudTab,
  unmarkActiveCloudTab,
  getTab,
} from "@app/worker/utils/tabs"
import { stateDidChange, whistState } from "@app/worker/utils/state"
import { getStorage } from "@app/worker/utils/storage"
import { Storage } from "@app/constants/storage"

import { PopupMessage, PopupMessageType } from "@app/@types/messaging"

merge(
  serverCookiesSynced,
  stateDidChange("waitingCloudTabs").pipe(
    withLatestFrom(socket),
    filter(
      ([change, socket]) =>
        change?.applyData?.name === "push" && socket.connected
    )
  )
)
  .pipe(withLatestFrom(socket))
  .subscribe(([, socket]: [any, Socket]) => {
    while (whistState.waitingCloudTabs.length > 0) {
      const tab = whistState.waitingCloudTabs.pop()

      if (tab !== undefined) {
        socket.emit("activate-tab", tab, true)
        markActiveCloudTab(tab)
      }

      void getActiveTab().then((activeTab) => {
        if (activeTab?.id === tab?.id) {
          void chrome.runtime.sendMessage(<PopupMessage>{
            type: PopupMessageType.SEND_POPUP_DATA,
            value: {
              isWaiting: false,
            },
          })
        }
      })
    }
  })

// If a tab is activated, active it on the server
merge(tabActivated, tabFocused)
  .pipe(withLatestFrom(serverCookiesSynced, socket))
  .subscribe(([tab, _synced, socket]: [chrome.tabs.Tab, any, Socket]) => {
    if (isCloudTab(tab)) {
      socket.emit("activate-tab", tab, false)
    }
  })

// If a tab is removed, remove it on the server
tabRemoved
  .pipe(withLatestFrom(socket))
  .subscribe(([tabId, socket]: [number, Socket]) => {
    const tab = find(whistState.activeCloudTabs, (t) => t.id === tabId)
    if (tab !== undefined) {
      socket.emit("close-tab", tab)
      unmarkActiveCloudTab(tab)
    }
  })

// If a user navigates away from a cloud tab, remove it on the server
tabUpdated
  .pipe(withLatestFrom(socket))
  .subscribe(async ([tab, socket]: [chrome.tabs.Tab, Socket]) => {
    const savedCloudUrls = await getStorage<string[]>(Storage.SAVED_CLOUD_URLS)
    const url = new URL(tab?.url?.replace("cloud:", "") ?? "")
    const host = url.hostname
    const isSaved = (savedCloudUrls ?? []).includes(host)

    if (!isCloudTab(tab) && !isSaved) {
      socket.emit("close-tab", tab)
      unmarkActiveCloudTab(tab)
    }
  })

tabZoomed
  .pipe(withLatestFrom(socket))
  .subscribe(([zoomChangeInfo, socket]: [object, Socket]) => {
    socket.emit("zoom-tab", zoomChangeInfo)
  })

webuiNavigate
  .pipe(withLatestFrom(socket))
  .subscribe(async ([message, socket]: [any, Socket]) => {
    const tab = await getTab(message.id)
    socket.emit("activate-tab", { ...tab, url: message.url }, true)
  })

webuiRefresh
  .pipe(withLatestFrom(socket))
  .subscribe(([message, socket]: [any, Socket]) => {
    socket.emit("refresh-tab", message)
  })

socketDisconnected.subscribe((socket: Socket) => {
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "SOCKET_DISCONNECTED",
    })
  )
})

socketConnected.subscribe(() => {
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "SOCKET_CONNECTED",
    })
  )
})
