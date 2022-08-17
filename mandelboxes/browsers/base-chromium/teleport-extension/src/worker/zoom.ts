import { Socket } from "socket.io-client"
import find from "lodash.find"

import { getOpenTabs } from "@app/utils/tabs"

const initZoomListener = (socket: Socket) => {
  socket.on(
    "zoom-tab",
    async ([zoomChangeInfo]: [{ newZoomFactor: number; tabId: number }]) => {
      const openTabs = await getOpenTabs()
      const foundTab = find(
        openTabs,
        (t) => t.clientTabId === zoomChangeInfo.tabId
      )

      if (foundTab?.tab.id !== undefined)
        chrome.tabs.setZoom(foundTab?.tab.id, zoomChangeInfo.newZoomFactor)
    }
  )
}

export { initZoomListener }
