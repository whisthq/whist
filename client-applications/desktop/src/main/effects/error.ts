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
  NoAccessError,
  UnauthorizedError,
  MandelboxError,
  AuthError,
  FractalError,
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
const errorWindow = (error: FractalError) => {
  closeWindows()
  createErrorWindow(error)
}

// For any failure, close all windows and display error window
onError(fromTrigger("mandelboxFlowFailure")).subscribe((x) => {
  if (mandelboxCreateErrorNoAccess(x)) {
    errorWindow(NoAccessError)
  } else if (mandelboxCreateErrorUnauthorized(x)) {
    errorWindow(UnauthorizedError)
  } else {
    errorWindow(MandelboxError)
  }
})

onError(fromTrigger("authFlowFailure")).subscribe(() => {
  errorWindow(AuthError)
})
