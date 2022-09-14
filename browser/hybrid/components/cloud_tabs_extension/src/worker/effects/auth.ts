import { initConfigTokenHandler } from "@app/worker/utils/auth"
import { authSuccess, authFailure } from "@app/worker/events/auth"
import { setStorage } from "@app/worker/utils/storage"
import { getActiveTab, updateTabUrl } from "@app/worker/utils/tabs"

import { PopupMessage, PopupMessageType } from "@app/@types/messaging"
import { AuthInfo } from "@app/@types/payload"
import { Storage } from "@app/constants/storage"

void initConfigTokenHandler()

// Tell the browser that auth succeeded and redirect to chrome://welcome
authSuccess.subscribe((auth: AuthInfo) => {
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
  })
})

// Tell the browser that auth failed
authFailure.subscribe(async (auth: AuthInfo) => {
  ;(chrome as any).whist.setWhistIsLoggedIn(false)
  chrome.runtime.sendMessage(<PopupMessage>{
    type: PopupMessageType.SEND_POPUP_DATA,
    value: {
      isLoggedIn: false,
    },
  })

  void setStorage(Storage.AUTH_INFO, auth)

  const { id } = await getActiveTab()
  updateTabUrl(id ?? -1, "chrome://welcome")
})
