/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */

import { Subject } from "rxjs"

import { ActionType, Action } from "@app/@types/actions"

export const eventTray = new Subject<Action>()

export const eventActionTypes = {
  signout: () => eventTray.next({ type: ActionType.SIGNOUT, payload: null }),
  quit: () => eventTray.next({ type: ActionType.QUIT, payload: null }),
}
