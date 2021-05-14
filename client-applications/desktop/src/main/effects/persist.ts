/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */
import { persist, clearKeys } from "@app/utils/persist"
import { StateIPC } from "@app/@types/state"
import { merge } from "rxjs"
import { containerCreateFailure } from "@app/main/observables/container"
import { protocolLaunchFailure } from "@app/main/observables/protocol"
import { tokenUpdateSuccess } from "@app/main/observables/auth"
import { signoutAction } from "@app/main/events/actions"

// User data is received from Auth0, which we persist

tokenUpdateSuccess.subscribe((state) => persist(state as Partial<StateIPC>))

// On certain kinds of failures (or on signout), we clear persistence to force the user
// to login again.
merge(
  containerCreateFailure,
  protocolLaunchFailure,
  signoutAction
).subscribe(() => clearKeys("refreshToken", "accessToken"))

// Uncomment this line to clear credentials in development.
// persistClear()
