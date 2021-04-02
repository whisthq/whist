/**
 * Copyright Fractal Computers, Inc. 2021
 * @file persist.ts
 * @brief This file contains RXJS observables that deal with state persistence (i.e. shared state).
*/

import { Observable } from "rxjs"
import { share } from "rxjs/operators"
import { store } from "@app/utils/persist"

export const fromEventPersist = (key: string) =>
    new Observable((subscriber) => {
        if (store.get(key) !== undefined) subscriber.next(store.get(key))
        store.onDidChange(key, (newKey, _oldKey) => {
            subscriber.next(newKey)
        })
    }).pipe(share())
