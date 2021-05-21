/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { merge, Observable, combineLatest } from "rxjs"
import { skipUntil, startWith, tap, map } from "rxjs/operators"
import { ChildProcess } from "child_process"

import {
  containerCreateErrorNoAccess,
  containerCreateErrorUnauthorized,
} from "@app/utils/container"
import { closeWindows } from "@app/utils/windows"
import {
  NoAccessError,
  UnauthorizedError,
  ProtocolError,
  ContainerError,
  AuthError,
  FractalError,
} from "@app/utils/error"
import { createErrorWindow } from "@app/utils/windows"
import { protocolStreamKill } from "@app/utils/protocol"
import { fromTrigger } from "@app/utils/flows"

const wait = (obs: Observable<any>) =>
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

const handleError = (error: FractalError) => {
  closeWindows()
  createErrorWindow(error)
}

// For any failure, close all windows and display error window
wait(fromTrigger("containerFlowFailure")).subscribe((x) => {
  if (containerCreateErrorNoAccess(x)) {
    handleError(NoAccessError)
  } else if (containerCreateErrorUnauthorized(x)) {
    handleError(UnauthorizedError)
  } else {
    handleError(ContainerError)
  }
})

wait(fromTrigger("protocolLaunchFlowFailure")).subscribe(() => {
  handleError(ProtocolError)
})

wait(
  merge(fromTrigger("loginFlowFailure"), fromTrigger("signupFlowFailure"))
).subscribe(() => {
  handleError(AuthError)
})
