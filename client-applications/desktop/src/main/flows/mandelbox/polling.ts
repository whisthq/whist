// This file is home to observables that manage mandelbox creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the mandelboxs while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success mandelbox creation emissions from here.

import { from, of, interval, merge } from "rxjs"
import {
    map,
    takeUntil,
    switchMap,
    take,
    mapTo,
    share,
    delay,
    sample,
} from "rxjs/operators"
import { some } from "lodash"

import {
    mandelboxPolling,
    mandelboxPollingIP,
    mandelboxPollingPorts,
    mandelboxPollingSecretKey,
    mandelboxPollingError,
    mandelboxPollingSuccess,
    mandelboxPollingPending,
} from "@app/utils/mandelbox"
import { mandelboxPollingTimeout } from "@app/utils/constants"
import { loadingFrom } from "@app/utils/observables"
import { fork, flow } from "@app/utils/flows"

const mandelboxPollingRequest = flow<{
    mandelboxID: string
    accessToken: string
}>("mandelboxPollingRequest", (trigger) =>
    fork(
        trigger.pipe(
            switchMap(({ mandelboxID, accessToken }) =>
                from(mandelboxPolling(mandelboxID, accessToken))
            )
        ),
        {
            success: (result) => mandelboxPollingSuccess(result),
            pending: (result) => mandelboxPollingPending(result),
            failure: (result) =>
                mandelboxPollingError(result) ||
                !some([
                    mandelboxPollingPending(result),
                    mandelboxPollingSuccess(result),
                ]),
        }
    )
)

const mandelboxPollingInner = flow<{
    mandelboxID: string
    accessToken: string
}>("mandelboxPollingInner", (trigger) => {
    const tick = trigger.pipe(
        switchMap((args) => interval(1000).pipe(mapTo(args)))
    )

    const limit = trigger.pipe(delay(mandelboxPollingTimeout))

    const poll = mandelboxPollingRequest(tick)

    const timeout = poll.pending.pipe(sample(limit)).pipe(take(1))

    return {
        pending: poll.pending.pipe(
            takeUntil(merge(poll.success, poll.failure))
        ),
        success: poll.success.pipe(take(1)),
        failure: merge(
            timeout.pipe(takeUntil(poll.success)),
            poll.failure.pipe(take(1))
        ),
    }
})

export default flow<{ mandelboxID: string; accessToken: string }>(
    "mandelboxPollingFlow",
    (trigger) => {
        const poll = trigger.pipe(
            map((args: { mandelboxID: string; accessToken: string }) =>
                mandelboxPollingInner(of(args))
            ),
            share()
        )

        const success = poll.pipe(switchMap((inner) => inner.success))
        const failure = poll.pipe(switchMap((inner) => inner.failure))
        const pending = poll.pipe(switchMap((inner) => inner.pending))
        const loading = loadingFrom(trigger, success, failure)

        // We probably won't subscribe to "pending" in the rest of the app,
        // so we subscribe to it deliberately here. If it has no subscribers,
        // it won't emit. We would like for it to emit for logging purposes.
        // We must remeber to takeUntil so it stops when we're finished.
        pending.pipe(takeUntil(merge(success, failure))).subscribe()

        return {
            success: success.pipe(
                map((response) => ({
                    mandelboxIP: mandelboxPollingIP(response),
                    mandelboxSecret: mandelboxPollingSecretKey(response),
                    mandelboxPorts: mandelboxPollingPorts(response),
                }))
            ),
            failure,
            pending,
            loading,
        }
    }
)
