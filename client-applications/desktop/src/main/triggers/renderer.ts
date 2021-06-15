import { Observable } from "rxjs"
import { filter, map, pluck } from "rxjs/operators"

import { createTrigger, fromTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

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

// // Fires when "Continue" button is clicked on error window popup
// createTrigger(
//   TRIGGER.relaunchAction,
//   filterByName(
//     fromTrigger("eventIPC").pipe(pluck("trigger")),
//     TRIGGER.relaunchAction
//   )
// )

// // Fires when "Signout" button is clicked on signout window popup
// createTrigger(
//   TRIGGER.clearCacheAction,
//   filterByName(
//     fromTrigger("eventIPC").pipe(pluck("trigger")),
//     TRIGGER.clearCacheAction
//   )
// )

// // Fires when "Signout" button is clicked on error window popup
// createTrigger(
//   TRIGGER.showSignoutWindow,
//   filterByName(
//     fromTrigger("eventIPC").pipe(pluck("trigger")),
//     TRIGGER.showSignoutWindow
//   )
// )

// // Fires when "Open Payment Information" button is clicked on error window popup
// createTrigger(
//   TRIGGER.showPaymentWindow,
//   filterByName(
//     fromTrigger("eventIPC").pipe(pluck("trigger")),
//     TRIGGER.showPaymentWindow
//   )
// );

;[
  TRIGGER.relaunchAction, // Fires when "Continue" button is clicked on error window popup
  TRIGGER.clearCacheAction, // Fires when "Signout" button is clicked on signout window popup
  TRIGGER.showSignoutWindow, // Fires when "Signout" button is clicked on error window popup
  TRIGGER.showPaymentWindow, // Fires when "Open Payment Information" button is clicked on error window popup
].forEach((trigger: string) => {
  createTrigger(
    trigger,
    filterByName(fromTrigger("eventIPC").pipe(pluck("trigger")), trigger)
  )
})
