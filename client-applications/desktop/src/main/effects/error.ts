/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { closeWindows } from "@app/utils/windows"
import {
  createErrorWindow,
  NoAccessError,
  NavigationError,
} from "@app/utils/error"
import { fromTrigger } from "@app/utils/flows"

// For any failure, close all windows and display error window
fromTrigger("failure").subscribe((payload: { name: string }) => {
  closeWindows()

  if (payload.name === "containerCreateFlowFailure") {
    createErrorWindow(NoAccessError)
  } else {
    createErrorWindow(NavigationError)
  }
})
