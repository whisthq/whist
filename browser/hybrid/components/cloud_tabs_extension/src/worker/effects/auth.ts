import { take } from "rxjs/operators"

import { initConfigTokenHandler } from "@app/worker/utils/auth"
import { authSuccess, authFailure } from "@app/worker/events/auth"
import { getStorage, setStorage } from "@app/worker/utils/storage"
import { getActiveTab, updateTabUrl } from "@app/worker/utils/tabs"
import { whistState } from "@app/worker/utils/state"

import { PopupMessage, PopupMessageType } from "@app/@types/messaging"
import { AuthInfo } from "@app/@types/payload"
import { Storage } from "@app/constants/storage"
import { welcomePageOpened } from "../events/webui"

void initConfigTokenHandler()

// Tell the browser that auth succeeded and redirect to chrome://welcome
authSuccess.subscribe(async (auth: AuthInfo) => {
  whistState.isLoggedIn = true

  if (auth.isFirstAuth) {
    const { id } = await getActiveTab()
    void updateTabUrl(id ?? -1, "chrome://welcome")
  }

  void chrome.runtime.sendMessage(<PopupMessage>{
    type: PopupMessageType.SEND_POPUP_DATA,
    value: {
      isLoggedIn: true,
    },
  })

  void setStorage(Storage.AUTH_INFO, auth)
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "IS_LOGGED_IN",
      value: {
        loggedIn: true,
      },
    })
  )
})

// Tell the browser that auth failed
authFailure.subscribe(async () => {
  whistState.isLoggedIn = false
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "IS_LOGGED_IN",
      value: {
        loggedIn: false,
      },
    })
  )

  void chrome.runtime.sendMessage(<PopupMessage>{
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
    void updateTabUrl(id ?? -1, "chrome://welcome")
})

welcomePageOpened.subscribe(() => {
  console.log("welcome page opened", whistState.isLoggedIn)
  ;(chrome as any).whist.broadcastWhistMessage(
    JSON.stringify({
      type: "IS_LOGGED_IN",
      value: {
        loggedIn: whistState.isLoggedIn,
      },
    })
  )
})
