/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */

import { mapValues } from 'lodash'
import { persist, persistClear } from '@app/utils/persist'
import { StateIPC } from '@app/utils/types'
import { combineLatest, merge } from 'rxjs'
import { startWith } from 'rxjs/operators'
import {
  userEmail,
  userConfigToken,
  userAccessToken,
  userRefreshToken
} from '@app/main/observables/user'

import { loginFailure } from '@app/main/observables/login'
import { signupFailure } from '@app/main/observables/signup'
import { containerCreateFailure } from '@app/main/observables/container'
import { protocolLaunchFailure } from '@app/main/observables/protocol'

// Persistence
// We create observables for each piece of state we want to persist.
// Each observable subscribes to the parts of the application that provide
// state updates.
//
// They are combined into a dictionary, which is persisted to local storage.

const subscribed = {
  userEmail,
  userConfigToken,
  userAccessToken,
  userRefreshToken
}

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
).subscribe(() => {
  console.log('Persistence Cleared!')
  persistClear()
})

// Uncomment this line to clear credentials in development.
// persistClear()
