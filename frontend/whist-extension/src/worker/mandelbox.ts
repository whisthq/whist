import { zip } from "rxjs"

import { createEvent, fromEvent } from "@app/utils/events"
import { mandelboxCreateSuccess, mandelboxRequest } from "@app/utils/mandelbox"

import { AuthInfo } from "@app/@types/payload"
import { AWSRegion } from "@app/constants/location"

import {
  ContentScriptMessageType,
  WorkerMessageType,
} from "@app/constants/messaging"

const initMandelboxAssign = () => {
  zip(
    fromEvent(WorkerMessageType.AUTH_SUCCESS),
    fromEvent(WorkerMessageType.CLOSEST_AWS_REGIONS)
  ).subscribe(async ([auth, regions]: [AuthInfo, AWSRegion[]]) => {
    console.log("Sending mandelbox request", auth, regions)
    const response = await mandelboxRequest(
      auth.accessToken,
      regions,
      auth.userEmail,
      chrome.runtime.getManifest().version
    )

    console.log("Response was", response)

    if (!mandelboxCreateSuccess(response)) return

    // Tell the application that mandlebox assign succeeded
    createEvent(WorkerMessageType.MANDELBOX_ASSIGN_SUCCESS, {
      ip: response.json.ip,
      mandelboxID: response.json.mandelbox_id,
    })
  })
}

export { initMandelboxAssign }
