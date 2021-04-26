// This file is home to observables that manage tray events.
// Their responsibilities are to listen events that will trigger tray actions.

import { debugObservables } from '@app/utils/logging'
import { identity } from 'lodash'
import { share, filter } from 'rxjs/operators'

import { fromAction, UserAction } from "@app/utils/actions"

const filterTray = (action: UserAction) => fromAction(action).pipe(filter(identity), share())

export const signoutRequest = filterTray(UserAction.SIGNOUT)
export const quitRequest = filterTray(UserAction.QUIT)

// Logging
debugObservables(
  [signoutRequest, 'signoutRequest'],
  [quitRequest, 'quitRequest']
)
