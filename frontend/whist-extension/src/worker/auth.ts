import has from "lodash.has"
import isEmpty from "lodash.isempty"

import { ipcMessage } from "@app/utils/messaging"
import { getCachedAuthInfo, refreshAuthInfo } from "@app/utils/auth"
import {
  authPortalURL,
  authInfoCallbackRequest,
  parseAuthInfo,
} from "@app/@core-ts/auth"
import { setStorage } from "@app/utils/storage"
import { createAuthTab, createLoggedInTab } from "@app/utils/tabs"
import { createEvent } from "@app/utils/events"

import { Storage } from "@app/constants/storage"
import {
  WorkerMessageType,
  ContentScriptMessageType,
} from "@app/constants/messaging"

const initWhistAuthHandler = async () => {
  /*
    Description:
      Checks cached auth tokens, refreshes them, and prompts the user to re-authenticate
      if necessary.
  */

  const authInfo = await getCachedAuthInfo()
  const refreshedAuthInfo = await refreshAuthInfo(authInfo)
  const wasAuthed = !isEmpty(authInfo) && authInfo?.refreshToken !== undefined

  // If the auth credentials are invalid, open a login page
  if (!wasAuthed || has(refreshedAuthInfo, "error")) {
    createAuthTab()
    return
  }

  // Otherwise, store the refreshed auth tokens
  setStorage(Storage.AUTH_INFO, JSON.stringify(refreshedAuthInfo))

  // Tell the application that auth succeeded
  createEvent(WorkerMessageType.AUTH_SUCCESS, refreshedAuthInfo)
}

const initGoogleAuthHandler = () => {
  /*
    Description:
      Opens the Google auth window when requested by the user
  */

  ipcMessage(ContentScriptMessageType.OPEN_GOOGLE_AUTH).subscribe(() => {
    chrome.identity.launchWebAuthFlow(
      {
        url: authPortalURL(),
        interactive: true,
      },
      async (callbackUrl) => {
        const response = (await authInfoCallbackRequest(callbackUrl)) as any
        const authInfo = parseAuthInfo(response)
        const refreshToken = response?.json.refresh_token

        console.log("storing", authInfo, response)

        if (!has(authInfo, "error")) {
          setStorage(
            Storage.AUTH_INFO,
            JSON.stringify({ ...authInfo, refreshToken })
          )
          createLoggedInTab()

          // Tell the application that auth succeeded
          createEvent(WorkerMessageType.AUTH_SUCCESS, authInfo)
        }

        // TODO: Show user a message on error
      }
    )
  })
}

export { initWhistAuthHandler, initGoogleAuthHandler }
