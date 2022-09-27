import { merge } from "rxjs"
import { withLatestFrom } from "rxjs/operators"
import find from "lodash.find"

import { authSuccess } from "@app/worker/events/auth"
import { webUiOpenSupport, webUiMouseEntered } from "@app/worker/events/webui"
import {
  tabActivated,
  tabRemoved,
  tabUpdated,
  tabCreated,
  tabFocused,
  cloudTabCreated,
  cloudTabUpdated,
  cloudTabActivated,
} from "@app/worker/events/tabs"
import { whistState } from "@app/worker/utils/state"
import {
  addTabToQueue,
  isActiveCloudTab,
  isCloudTab,
  removeTabFromQueue,
  unmarkActiveCloudTab,
  stripCloudUrl,
  updateTabUrl,
  getTab,
} from "@app/worker/utils/tabs"
import { toBase64 } from "@app/worker/utils/encode"
import { cannotStreamError } from "@app/worker/utils/errors"
import { serializeProtocolArgs } from "@app/worker/utils/protocol"
import { createOrFocusHelpPopup } from "@app/worker/utils/help"
import { helpScreenOpened } from "@app/worker/events/messages"
import { HelpScreenMessageType } from "@app/@types/messaging"

// On launch, add all previously opened cloud tabs to the cloud tab queue
authSuccess.subscribe(() => {
  chrome.tabs.query({}, (tabs: chrome.tabs.Tab[]) => {
    // Tech debt: Change this when server-side multiwindow is supported
    // Currently, because the server only has one window, if there are restored
    // cloud tabs spread across multiple windows, we have no way of sending the correct
    // video frames to each restored tab since only one window can be visible at a time
    // on the server. So, if we detect restored tabs across multiple windows, we
    // revert them to local tabs as a temporary fix.
    const cloudTabs = tabs.filter((tab) => tab.url?.startsWith("cloud:"))
    const sameWindow = tabs.every((tab) => tab.windowId === tabs[0].windowId)

    if (!sameWindow) {
      cloudTabs.forEach((tab) => {
        if (tab.url !== undefined)
          void updateTabUrl(tab.id, stripCloudUrl(tab.url))
      })
    } else {
      cloudTabs.forEach((tab) => {
        if (!isActiveCloudTab(tab)) addTabToQueue(tab)
      })
    }
  })
})

// If a tab is created or updated and it's a cloud tab, add it to the queue
merge(tabCreated, tabUpdated)
  .pipe(withLatestFrom(authSuccess))
  .subscribe(([tab, _auth]: [chrome.tabs.Tab, any]) => {
    if (isCloudTab(tab) && !isActiveCloudTab(tab)) addTabToQueue(tab)
  })

// If a tab is activated or changed, update its info in the cloud tab queue
merge(tabActivated, tabUpdated, tabFocused)
  .pipe(withLatestFrom(authSuccess))
  .subscribe(async ([tab, _auth]: [chrome.tabs.Tab, any]) => {
    whistState.waitingCloudTabs = await Promise.all(
      whistState.waitingCloudTabs.map(async (tab) => await getTab(tab.id))
    )

    if (!isCloudTab(tab)) unmarkActiveCloudTab(tab)
  })

merge(tabActivated, tabUpdated, webUiMouseEntered).subscribe(
  (tab: chrome.tabs.Tab) => {
    if (isCloudTab(tab)) {
      ;(chrome as any).whist.broadcastWhistMessage(
        JSON.stringify({
          type: "CHANGE_FOCUSED_TAB",
          value: {
            id: tab.id,
          },
        })
      )
    }
  }
)

// If a tab is removed, remove it from the cloud tab queue
tabRemoved.subscribe((tabId: number) => {
  const tab = find(whistState.waitingCloudTabs, (t) => t.id === tabId)
  if (tab !== undefined) removeTabFromQueue(tab)
})

tabUpdated.subscribe((tab: chrome.tabs.Tab) => {
  if (isCloudTab(tab) && whistState.mandelboxInfo !== undefined) {
    const s = serializeProtocolArgs(whistState.mandelboxInfo)

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
  }
})

tabActivated.subscribe((tab: chrome.tabs.Tab) => {
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "TAB_ACTIVATED",
      value: {
        id: tab.id,
      },
    })
  )
})

cloudTabUpdated.subscribe(
  async ([tabId, _historyLength, tab]: [number, number, chrome.tabs.Tab]) => {
    if (tabId === undefined || cannotStreamError(tab) !== undefined) return

    if (tab.favIconUrl !== undefined) {
      tab.favIconUrl = await toBase64(tab.favIconUrl)
      ;(chrome as any).whist.broadcastWhistMessage(
        JSON.stringify({
          type: "UPDATE_TAB",
          value: {
            id: tabId,
            favIconUrl: tab?.favIconUrl,
          },
        })
      )
    }
  }
)

cloudTabUpdated.subscribe(
  ([tabId, historyLength, tab]: [number, number, chrome.tabs.Tab]) => {
    if (tabId === undefined || cannotStreamError(tab) !== undefined) return
    ;(chrome as any).whist.broadcastWhistMessage(
      JSON.stringify({
        type: "UPDATE_TAB",
        value: {
          id: tabId,
          ...(tab.url !== undefined && {
            url: `cloud:${stripCloudUrl(tab.url)}`,
          }),
          title: tab?.title,
          historyLength,
        },
      })
    )
  }
)

cloudTabUpdated.subscribe(
  ([tabId, _historyLength, tab]: [number, number, chrome.tabs.Tab]) => {
    if (tab.url !== undefined && cannotStreamError(tab) !== undefined) {
      void updateTabUrl(tabId, stripCloudUrl(tab.url))
    }
  }
)

cloudTabCreated.subscribe((tabs: chrome.tabs.Tab[]) => {
  const tab = tabs[0]

  if (tab.url !== undefined)
    void chrome.tabs.create({ url: tab.url, active: tab.active })
})

webUiOpenSupport.subscribe(createOrFocusHelpPopup)

helpScreenOpened.pipe(withLatestFrom(authSuccess)).subscribe(([_, auth]) => {
  void chrome.runtime.sendMessage({
    type: HelpScreenMessageType.HELP_SCREEN_USER_EMAIL,
    value: auth.userEmail,
  })
})

cloudTabActivated.subscribe(([tabId, spotlightId]: [number, number]) => {
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "ACTIVATE_TAB",
      value: {
        id: tabId,
        spotlightId,
      },
    })
  )
})
