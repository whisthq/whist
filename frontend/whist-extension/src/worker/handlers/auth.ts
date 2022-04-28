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
import { createAuthTab } from "@app/worker/utils/tabs"

import { cachedAuthInfo } from "@app/constants/storage"
import { openGoogleAuth } from "@app/constants/messaging"

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
  await setStorage(cachedAuthInfo, JSON.stringify(refreshedAuthInfo))
}

const initGoogleAuthHandler = () => {
  /*
    Description:
      Opens the Google auth window when requested by the user
  */

  ipcMessage(openGoogleAuth).subscribe(() => {
    chrome.identity.launchWebAuthFlow(
      {
        url: authPortalURL(),
        interactive: true,
      },
      async (callbackUrl) => {
        const response = (await authInfoCallbackRequest(callbackUrl)) as any
        const authInfo = parseAuthInfo(response)

        if (!has(authInfo, "error"))
          setStorage(cachedAuthInfo, JSON.stringify(authInfo))

        // TODO: Show user a message on error
      }
    )
  })
}

export { initWhistAuthHandler, initGoogleAuthHandler }
