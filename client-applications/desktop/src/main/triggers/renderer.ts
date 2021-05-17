import { Observable } from "rxjs"
import { filter, map, pluck } from "rxjs/operators"

import { createTrigger, fromTrigger } from "@app/utils/flows"

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

// Fires when login button is clicked
createTrigger(
  "login",
  filterByName(fromTrigger("eventIPC").pipe(pluck("trigger")), "login")
)
// Fires when signup button is clicked
createTrigger(
  "signup",
  filterByName(fromTrigger("eventIPC").pipe(pluck("trigger")), "signup")
)
// Fires when "Continue" button is clicked on error window popup
createTrigger(
  "relaunch",
  filterByName(fromTrigger("eventIPC").pipe(pluck("trigger")), "relaunch")
)
