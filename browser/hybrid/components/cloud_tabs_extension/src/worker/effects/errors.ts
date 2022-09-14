import { merge } from "rxjs"
import { filter } from "rxjs/operators"

import { mandelboxError } from "@app/worker/events/mandelbox"
import { hostError } from "@app/worker/events/host"
import { authNetworkError } from "@app/worker/events/auth"

import { whistState } from "@app/worker/utils/state"
import { stripCloudUrl, updateTabUrl } from "@app/worker/utils/tabs"
import {
  mandelboxCreateErrorCommitHash,
  mandelboxRequest,
} from "@app/worker/utils/mandelbox"

import { AsyncReturnType } from "@app/@types/api"

const showErrorPopup = (type: string) => {
  whistState.waitingCloudTabs.forEach((tab) => {
    updateTabUrl(tab.id, stripCloudUrl(tab.url ?? ""), () => {
      setTimeout(() => {
        chrome.tabs.sendMessage(tab.id ?? 0, { type })
      }, 1000)
    })
  })
  whistState.waitingCloudTabs = []
}

merge(
  mandelboxError.pipe(
    filter(
      (response: AsyncReturnType<typeof mandelboxRequest>) =>
        !mandelboxCreateErrorCommitHash(response)
    )
  ),
  hostError,
  authNetworkError
).subscribe(() => {
  showErrorPopup("SERVER_ERROR")
})

mandelboxError
  .pipe(
    filter((response: AsyncReturnType<typeof mandelboxRequest>) =>
      mandelboxCreateErrorCommitHash(response)
    )
  )
  .subscribe(() => {
    showErrorPopup("UPDATE_NEEDED")
  })
