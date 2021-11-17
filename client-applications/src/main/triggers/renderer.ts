import { Observable } from "rxjs"
import { filter, map, pluck } from "rxjs/operators"

import { createTrigger, fromTrigger } from "@app/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

const filterByName = (
  observable: Observable<{ name: string; payload: any }>,
  name: string
) =>
  observable.pipe(
    filter(
      (x: { name: string; payload: any }) => x !== undefined && x.name === name
    ),
    map((x: { name: string; payload: any }) => x.payload)
  )

;[
  WhistTrigger.relaunchAction, // Fires when "Continue" button is clicked on error window popup
  WhistTrigger.clearCacheAction, // Fires when "Signout" button is clicked on signout window popup
  WhistTrigger.showSignoutWindow, // Fires when "Signout" button is clicked on error window popup
  WhistTrigger.showPaymentWindow, // Fires when "Open Payment Information" button is clicked on error window popup
  WhistTrigger.onboarded,
  WhistTrigger.emitIPC,
].forEach((trigger: string) => {
  createTrigger(
    trigger,
    filterByName(
      fromTrigger(WhistTrigger.eventIPC).pipe(pluck("trigger")),
      trigger
    )
  )
})
