import { take } from "rxjs/operators"

import { initConfigTokenHandler } from "@app/worker/utils/auth"
import { authSuccess, authFailure } from "@app/worker/events/auth"
import { getStorage, setStorage } from "@app/worker/utils/storage"
import { getActiveTab, updateTabUrl } from "@app/worker/utils/tabs"
import { whistState } from "@app/worker/utils/state"

import { PopupMessage, PopupMessageType } from "@app/@types/messaging"
import { AuthInfo } from "@app/@types/payload"
import { Storage } from "@app/constants/storage"

void initConfigTokenHandler()

// Tell the browser that auth succeeded and redirect to chrome://welcome
authSuccess.subscribe((auth: AuthInfo) => {
  whistState.isLoggedIn = true
  ;(chrome as any).whist.setWhistIsLoggedIn(true, async () => {
    if (auth.isFirstAuth) {
      const { id } = await getActiveTab()
      updateTabUrl(id ?? -1, "chrome://welcome")
    }

    chrome.runtime.sendMessage(<PopupMessage>{
      type: PopupMessageType.SEND_POPUP_DATA,
      value: {
        isLoggedIn: true,
      },
    })

    void setStorage(Storage.AUTH_INFO, auth)
    whistState.isLoggedIn = true
  })
})

// Tell the browser that auth failed
authFailure.subscribe(async () => {
  whistState.isLoggedIn = false
  ;(chrome as any).whist.setWhistIsLoggedIn(false)
  chrome.runtime.sendMessage(<PopupMessage>{
    type: PopupMessageType.SEND_POPUP_DATA,
    value: {
      isLoggedIn: false,
    },
  })
})

authFailure.pipe(take(1)).subscribe(async () => {
  const { id } = await getActiveTab()
  const authInfo = await getStorage<AuthInfo>(Storage.AUTH_INFO)

  if (authInfo?.accessToken === undefined)
    updateTabUrl(id ?? -1, "chrome://welcome")
})
