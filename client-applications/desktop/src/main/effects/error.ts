/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { Observable, combineLatest } from "rxjs"
import { skipUntil, startWith, tap, map } from "rxjs/operators"
import { ChildProcess } from "child_process"

import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
} from "@app/utils/mandelbox"
import { closeWindows, createErrorWindow } from "@app/utils/windows"
import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
  PROTOCOL_ERROR,
} from "@app/utils/error"
import { protocolStreamKill } from "@app/utils/protocol"
import { fromTrigger } from "@app/utils/flows"

// Wrapper function that waits for Electron to have loaded (so the error window can appear)
// and kills the protocol (so the protocol isn't still running if there's an error)
const onError = (obs: Observable<any>) =>
  combineLatest(
    obs,
    fromTrigger("protocolLaunchFlowSuccess").pipe(startWith(undefined))
  ).pipe(
    skipUntil(fromTrigger("appReady")),
    tap(([, protocol]: [any, ChildProcess]) => {
      protocolStreamKill(protocol)
    }),
    map(([x]: [any, ChildProcess]) => x)
  )

// Closees all windows and creates the error window
const errorWindow = (hash: string) => {
  closeWindows()
  createErrorWindow(hash)
}

// For any failure, close all windows and display error window
onError(fromTrigger("mandelboxFlowFailure")).subscribe((x) => {
  if (mandelboxCreateErrorNoAccess(x)) {
    errorWindow(NO_PAYMENT_ERROR)
  } else if (mandelboxCreateErrorUnauthorized(x)) {
    errorWindow(UNAUTHORIZED_ERROR)
  } else {
    errorWindow(MANDELBOX_INTERNAL_ERROR)
  }
})

onError(fromTrigger("authFlowFailure")).subscribe(() => {
  errorWindow(AUTH_ERROR)
})

onError(fromTrigger("protocolCloseFlowSuccess")).subscribe((code: number) => {
  if (code !== 0) errorWindow(PROTOCOL_ERROR)
})
