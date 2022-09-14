import { mandelboxError } from "@app/worker/events/mandelbox"
import { hostError } from "@app/worker/events/host"
import { authNetworkError } from "@app/worker/events/auth"
import { merge } from "rxjs"

import { whistState } from "@app/worker/utils/state"
import { stripCloudUrl, updateTabUrl } from "@app/worker/utils/tabs"

merge(mandelboxError, hostError, authNetworkError).subscribe(() => {
  whistState.waitingCloudTabs.forEach((tab) => {
    updateTabUrl(tab.id, stripCloudUrl(tab.url ?? ""), () => {
      setTimeout(() => {
        chrome.tabs.sendMessage(tab.id ?? 0, { type: "SERVER_ERROR" })
      }, 1000)
    })
  })
  whistState.waitingCloudTabs = []
})
