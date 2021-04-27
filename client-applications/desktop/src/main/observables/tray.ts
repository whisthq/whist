// This file is home to observables that manage tray events.
// Their responsibilities are to listen events that will trigger tray actions.

import { fromEventTray } from "@app/main/events/tray"
import { debugObservables } from "@app/utils/logging"
import { identity } from "lodash"
import { share, filter } from "rxjs/operators"

export const signoutRequest = fromEventTray("signoutRequest").pipe(
  filter(identity),
  share()
)

export const quitRequest = fromEventTray("quitRequest").pipe(
  filter(identity),
  share()
)

// Logging

debugObservables(
  [signoutRequest, "signoutRequest"],
  [quitRequest, "quitRequest"]
)
