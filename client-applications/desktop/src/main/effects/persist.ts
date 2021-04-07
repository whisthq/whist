/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */

import { mapValues } from "lodash"
import { persist } from "@app/utils/persist"
import { combineLatest } from "rxjs"
import { startWith } from "rxjs/operators"
import {
    userEmail,
    userConfigToken,
    userAccessToken,
    userRefreshToken,
} from "@app/main/observables/user"

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
    userRefreshToken,
}

combineLatest(
    mapValues(subscribed, (o: any): any => o.pipe(startWith(undefined)))
).subscribe((state) => persist(state))
