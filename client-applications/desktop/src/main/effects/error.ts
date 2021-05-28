/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { Observable, combineLatest } from "rxjs"
import { skipUntil, startWith, tap, map } from "rxjs/operators"
import { ChildProcess } from "child_process"

import {
  containerCreateErrorNoAccess,
  containerCreateErrorUnauthorized,
} from "@app/utils/container"
import { closeWindows, createErrorWindow } from "@app/utils/windows"
import {
  NoAccessError,
  UnauthorizedError,
  ProtocolError,
  ContainerError,
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
const handleError = (error: FractalError) => {
  closeWindows()
  createErrorWindow(error)
}

// For any failure, close all windows and display error window
onError(fromTrigger("containerFlowFailure")).subscribe((x) => {
  if (containerCreateErrorNoAccess(x)) {
    handleError(NoAccessError)
  } else if (containerCreateErrorUnauthorized(x)) {
    handleError(UnauthorizedError)
  } else {
    handleError(ContainerError)
  }
})

onError(fromTrigger("protocolLaunchFlowFailure")).subscribe(() => {
  handleError(ProtocolError)
})

onError(fromTrigger("authFlowFailure")).subscribe(() => {
  handleError(AuthError)
})
