/**
 * Copyright Fractal Computers, Inc. 2021
 * @file persist.ts
 * @brief This file contains RXJS observables that deal with state persistence (i.e. shared state).
 */

import { eventAppReady } from "@app/main/events/app"
import { Subject } from "rxjs"
import { distinctUntilChanged, map } from "rxjs/operators"
import { store } from "@app/utils/persist"

// We create the persistence observables slightly differently from the
// others across the project.
//
// A Subject is "multicast" by default, and we can also "push" values onto
// it by calling its "next()" method. This is important here, because we don't
// want to load the persisted storage right away. We must wait for eventAppReady,
// because we want all the observable subscriptions throughout the project to
// be setup. If they are not set up and we load the persisted storage too early,
// then they will not receive the data from this initial "load" event, and they
// will never emit.
//
// So we need to define "persisted", because we need to import it into other
// file, but we can't load storage into right away. This means we need a way
// to "push" the loaded storage onto the "persisted" observable. Hence, we use
// a Subject instead of an Observable as the constructor.
//
// fromEventPersist is the interface that observables across the project should
// use. It picks a single key from persisted storage, and emits only when the
// value at that key has changed.
//
// We deliberated create "persisted" outside of fromEventPersisted, because
// we don't want to set up a new EventEmitter object for every subscriber to
// fromEventPersist. We want to use the same EventEmitter, otherwise we open
// ourselves up to memory leaks.

export const persisted = new Subject()

export const fromEventPersist = (key: string) =>
    persisted.pipe(
        map((obj: any) => obj[key]),
        distinctUntilChanged()
    )

store.onDidAnyChange((newStore: any, _oldStore: any) => {
    persisted.next(newStore)
})

eventAppReady.subscribe(() => {
    persisted.next(store.store)
})
