import { loadingFrom } from "@app/utils/observables"
import {
    hostServiceInfo,
    hostServiceInfoValid,
    hostServiceInfoIP,
    hostServiceInfoPort,
    hostServiceInfoPending,
    hostServiceInfoSecret,
    hostServiceConfig,
    hostServiceConfigValid,
    hostServiceConfigError,
    HostServiceInfoResponse,
    HostServiceConfigResponse,
} from "@app/utils/host"
import { from, interval, of, merge, zip } from "rxjs"
import {
    map,
    share,
    takeUntil,
    switchMap,
    mapTo,
    take,
    filter,
} from "rxjs/operators"
import { flow } from "@app/utils/flows"
import { some, pick } from "lodash"

const hostServiceInfoRequest = flow<{
    jwtIdentity: string
    accessToken: string
    configToken: string
}>("hostServiceInfoRequest", (trigger) => {
    const request = trigger.pipe(
        switchMap(({ jwtIdentity, accessToken }) =>
            from(hostServiceInfo(jwtIdentity, accessToken))
        )
    )
    return {
        success: request.pipe(filter((result) => hostServiceInfoValid(result))),
        pending: request.pipe(
            filter((result) => hostServiceInfoPending(result))
        ),
        failure: request.pipe(
            filter(
                (result) =>
                    !some([
                        hostServiceInfoValid(result),
                        hostServiceInfoPending(result),
                    ])
            )
        ),
    }
})

const hostPollingInner = flow<{
    jwtIdentity: string
    accessToken: string
    configToken: string
}>("hostPollingInner", (trigger) => {
    const tick = trigger.pipe(
        switchMap((args) => interval(1000).pipe(mapTo(args)))
    )
    const poll = hostServiceInfoRequest(tick)

    return {
        pending: poll.pending.pipe(
            takeUntil(merge(poll.success, poll.failure))
        ),
        success: poll.success.pipe(take(1)),
        failure: poll.failure.pipe(takeUntil(poll.success), take(1)),
    }
})

const hostInfoFlow = flow<{
    jwtIdentity: string
    accessToken: string
    configToken: string
}>("hostInfoFlow", (trigger) => {
    const poll = trigger.pipe(
        map((args) => hostPollingInner(of(args))),
        share()
    )

    const success = poll.pipe(switchMap((inner) => inner.success))
    const failure = poll.pipe(switchMap((inner) => inner.failure))
    const pending = poll.pipe(switchMap((inner) => inner.pending))
    const loading = loadingFrom(trigger, success, failure)

    return {
        success: success.pipe(
            map((response) => ({
                mandelboxIP: hostServiceInfoIP(response),
                mandelboxPort: hostServiceInfoPort(response),
                mandelboxSecret: hostServiceInfoSecret(response),
            }))
        ),
        failure,
        pending,
        loading,
    }
})

const hostConfigFlow = flow<{
    mandelboxIP: string
    mandelboxPort: number
    mandelboxSecret: string
    jwtIdentity: string
    configToken: string
    accessToken: string
}>("hostConfigFlow", (trigger) => {
    const hostServiceConfigRequest = trigger.pipe(
        switchMap(
            ({
                mandelboxIP,
                mandelboxPort,
                mandelboxSecret,
                jwtIdentity,
                configToken,
                accessToken,
            }) =>
                from(
                    hostServiceConfig(
                        mandelboxIP,
                        mandelboxPort,
                        mandelboxSecret,
                        jwtIdentity,
                        configToken,
                        accessToken
                    )
                )
        )
    )
    return {
        failure: hostServiceConfigRequest.pipe(
            filter((result) => hostServiceConfigError(result))
        ),
        success: hostServiceConfigRequest.pipe(
            filter((result) => hostServiceConfigValid(result))
        ),
    }
})

export default flow<{
    jwtIdentity: string
    accessToken: string
    configToken: string
}>("hostServiceFlow", (trigger) => {
    const info = hostInfoFlow(trigger)

    const config = hostConfigFlow(
        zip(trigger, info.success).pipe(
            map(([t, i]) => ({
                ...pick(t, ["jwtIdentity", "configToken", "accessToken"]),
                ...pick(i, ["mandelboxIP", "mandelboxPort", "mandelboxSecret"]),
            }))
        )
    )

    return {
        success: config.success,
        failure: merge(info.failure, config.failure),
    }
})
