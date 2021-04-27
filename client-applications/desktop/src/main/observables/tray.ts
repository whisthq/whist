// This file is home to observables that manage tray events.
// Their responsibilities are to listen events that will trigger tray actions.

import { debugObservables } from "@app/utils/logging"
import { share } from "rxjs/operators"

import { action } from "@app/main/events/actions"
import { TrayAction } from "@app/@types/actions"

export const signoutRequest = action(TrayAction.SIGNOUT).pipe(share())
export const quitRequest = action(TrayAction.QUIT).pipe(share())

// Logging
debugObservables(
  [signoutRequest, "signoutRequest"],
  [quitRequest, "quitRequest"]
)
