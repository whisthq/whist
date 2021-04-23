/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */
import { Subject } from "rxjs"
import { filter } from "rxjs/operators"
import { identity } from "lodash"

type TrayAction = "signoutRequest" | "quitRequest"
export const eventTray = new Subject()
export const eventTrayActions = {
  signout: () => eventTray.next("signoutRequest"),
  quit: () => eventTray.next("quitRequest"),
}
export const fromEventTray = (action: TrayAction) =>
  eventTray.pipe(filter((request) => request == action))
