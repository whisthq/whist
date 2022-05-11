import has from "lodash.has"
import isEmpty from "lodash.isempty"

import { ipcMessage } from "@app/worker/utils/messaging"
import { getCachedAuthInfo, refreshAuthInfo } from "@app/worker/utils/auth"
import {
  authPortalURL,
  authInfoCallbackRequest,
  parseAuthInfo,
} from "@app/worker/@core-ts/auth"
import { setStorage } from "@app/worker/utils/storage"
import { createAuthTab, createLoggedInTab } from "@app/worker/utils/tabs"

import { Storage } from "@app/constants/storage"
import { ContentScriptMessageType } from "@app/constants/messaging"

const initWhistAuthHandler = async () => {
  /*
    Description:
      Checks cached auth tokens, refreshes them, and prompts the user to re-authenticate
      if necessary.
  */

  const authInfo = await getCachedAuthInfo()

  const wasAuthed = !isEmpty(authInfo) && authInfo?.refreshToken !== undefined
  const refreshedAuthInfo = refreshAuthInfo(authInfo)

  // If the auth credentials are invalid, open a login page
  if (!wasAuthed || has(refreshedAuthInfo, "error")) {
    createAuthTab()
    return
  }

  // Otherwise, store the refreshed auth tokens
  setStorage(Storage.AUTH_INFO, JSON.stringify(refreshedAuthInfo))

  // Tell the application that auth succeeded
  chrome.runtime.sendMessage({
    type: ContentScriptMessageType.AUTH_SUCCESS,
    value: refreshedAuthInfo,
  })
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

        if (!has(authInfo, "error")) {
          setStorage(Storage.AUTH_INFO, JSON.stringify(authInfo))
          createLoggedInTab()

          // Tell the application that auth succeeded
          chrome.runtime.sendMessage({
            type: ContentScriptMessageType.AUTH_SUCCESS,
          })
        }

        // TODO: Show user a message on error
      }
    )
  })
}

export { initWhistAuthHandler, initGoogleAuthHandler }
