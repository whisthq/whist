/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */

import { mapValues } from "lodash"
import { persist, persistClear } from "@app/utils/persist"
import { StateIPC } from "@app/@types/state"
import { combineLatest, merge } from "rxjs"
import { startWith } from "rxjs/operators"
import {
  userEmail,
  userConfigToken,
  userAccessToken,
  userRefreshToken,
} from "@app/main/observables/user"

import { loginFailure } from "@app/main/observables/login"
import { signupFailure } from "@app/main/observables/signup"
import { containerCreateFailure } from "@app/main/observables/container"
import { protocolLaunchFailure } from "@app/main/observables/protocol"

// We create observables for each piece of state we want to persist.
// Each observable subscribes to the parts of the application that provide
// state updates.

const subscribed = {
  userEmail,
  userConfigToken,
  userAccessToken,
  userRefreshToken,
}

// We combined the "subscribed" observables into a dictionary, using their names
// as the dictionary keys. This is the object that is persisted to local storage.
combineLatest(
  mapValues(subscribed, (o: any): any => o.pipe(startWith(undefined)))
).subscribe((state) => persist(state as Partial<StateIPC>))

// On certain kinds of failures, we clear persistence to force the user
// to login again.
merge(
  loginFailure,
  signupFailure,
  containerCreateFailure,
  protocolLaunchFailure
).subscribe(() => persistClear())

// Uncomment this line to clear credentials in development.
// persistClear()
