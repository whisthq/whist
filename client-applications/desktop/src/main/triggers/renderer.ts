import { Observable } from "rxjs"
import { filter, map, pluck } from "rxjs/operators"

import { createTrigger, fromTrigger } from "@app/main/utils/flows"

const filterByName = (
  observable: Observable<{ name: string; payload: any }>,
  name: string
) =>
  observable.pipe(
    filter(
      (x: { name: string; payload: any }) =>
        x !== undefined && x.name === name
    ),
    map((x: { name: string; payload: any }) => x.payload)
  )

createTrigger(
  "login",
  filterByName(fromTrigger("eventIPC").pipe(pluck("trigger")), "login")
)
createTrigger(
  "signup",
  filterByName(fromTrigger("eventIPC").pipe(pluck("trigger")), "signup")
)
createTrigger(
  "relaunch",
  filterByName(fromTrigger("eventIPC").pipe(pluck("trigger")), "relaunch")
)
