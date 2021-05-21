/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { merge, Observable } from "rxjs"
import { skipUntil } from "rxjs/operators"

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
import { 
  createErrorWindow 
} from "@app/utils/windows"
import { fromTrigger } from "@app/utils/flows"

const wait = (obs: Observable<any>) =>
  obs.pipe(skipUntil(fromTrigger("appReady")))

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
