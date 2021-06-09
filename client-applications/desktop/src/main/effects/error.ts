/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { Observable, combineLatest } from "rxjs"
import { skipUntil, tap, map } from "rxjs/operators"
import { ChildProcess } from "child_process"

import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
} from "@app/utils/mandelbox"
import { createErrorWindow } from "@app/utils/windows"
import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
} from "@app/utils/error"
import { protocolStreamKill } from "@app/utils/protocol"
import { fromTrigger } from "@app/utils/flows"

// Wrapper function that waits for Electron to have loaded (so the error window can appear)
// and kills the protocol (so the protocol isn't still running if there's an error)
const onError = (obs: Observable<any>) =>
  combineLatest(obs, fromTrigger("childProcessSpawn")).pipe(
    skipUntil(fromTrigger("appReady")),
    tap(([, protocol]: [any, ChildProcess]) => {
      protocolStreamKill(protocol)
    }),
    map(([x]: [any, ChildProcess]) => x)
  )

// For any failure, close all windows and display error window
onError(fromTrigger("mandelboxFlowFailure")).subscribe((x: any) => {
  if (mandelboxCreateErrorNoAccess(x)) {
    createErrorWindow(NO_PAYMENT_ERROR)
  } else if (mandelboxCreateErrorUnauthorized(x)) {
    createErrorWindow(UNAUTHORIZED_ERROR)
  } else {
    createErrorWindow(MANDELBOX_INTERNAL_ERROR)
  }
})

onError(fromTrigger("authFlowFailure")).subscribe(() => {
  createErrorWindow(AUTH_ERROR)
})
