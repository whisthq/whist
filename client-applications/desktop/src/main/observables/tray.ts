// This file is home to observables that manage tray events.
// Their responsibilities are to listen events that will trigger tray actions.

import { debugObservables } from "@app/utils/logging"
import { share } from "rxjs/operators"

import { fromAction } from "@app/utils/actions"
import { TrayAction } from "@app/@types/actions"

const filterTray = (action: TrayAction) => fromAction(action).pipe(share())

export const signoutRequest = filterTray(TrayAction.SIGNOUT)
export const quitRequest = filterTray(TrayAction.QUIT)

// Logging
debugObservables(
  [signoutRequest, "signoutRequest"],
  [quitRequest, "quitRequest"]
)
