import { zip } from "rxjs"

import { ipcMessage } from "@app/worker/utils/messaging"
import {
  mandelboxCreateSuccess,
  mandelboxRequest,
} from "@app/worker/utils/mandelbox"

import { AuthInfo } from "@app/@types/auth"
import { AWSRegion } from "@app/@types/aws"

import { ContentScriptMessageType } from "@app/constants/messaging"

const initMandelboxAssign = () => {
  zip(
    ipcMessage(ContentScriptMessageType.AUTH_SUCCESS),
    ipcMessage(ContentScriptMessageType.CLOSEST_AWS_REGIONS)
  ).subscribe(async ([args, regions]: [AuthInfo, AWSRegion[]]) => {
    const response = await mandelboxRequest(
      args.accessToken,
      regions,
      args.userEmail,
      chrome.runtime.getManifest().version
    )

    if (!mandelboxCreateSuccess(response)) return

    // Tell the application that mandlebox assign succeeded
    chrome.runtime.sendMessage({
      type: ContentScriptMessageType.MANDELBOX_ASSIGN_SUCCESS,
      value: {
        ip: response.json.ip,
        mandelboxID: response.json.mandelbox_id,
      },
    })
  })
}

export { initMandelboxAssign }
