/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { app } from "electron"
import { closeWindows } from "@app/utils/windows"
import { clearKeys } from "@app/utils/persist"

import {
  errorRelaunchRequest,
  errorWindowRequest,
} from "@app/main/observables/error"

// Other parts of the application need to know that an error has happened,
// which is why we have observables like "errorWindowRequest" defined outside
// of the Effects module of the application.

errorRelaunchRequest.subscribe(() => {
  app.relaunch()
  app.exit()
})

errorWindowRequest.subscribe((windowFunction) => {
  closeWindows()
  windowFunction()
})
