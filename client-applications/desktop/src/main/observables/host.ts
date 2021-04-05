import {
    userEmail,
    userAccessToken,
    userConfigToken,
} from "@app/main/observables/user"
import { containerAssignPolling } from "@app/main/observables/container"
import { loadingFrom } from "@app/utils/observables"
import {
    hostServiceInfo,
    hostServiceInfoValid,
    hostServiceInfoError,
    hostServiceInfoIP,
    hostServiceInfoPort,
    hostServiceInfoSecret,
    hostServiceConfig,
} from "@app/utils/api"
import { from } from "rxjs"
import {
    map,
    take,
    share,
    filter,
    skipWhile,
    exhaustMap,
    withLatestFrom,
} from "rxjs/operators"

export const hostInfoRequest = containerAssignPolling.pipe(
    exhaustMap((poll) =>
        poll.pipe(
            skipWhile((res) => res?.json.state !== "PENDING"),
            take(1)
        )
    ),
    withLatestFrom(userEmail, userAccessToken),
    map(([_, email, token]) => [email, token]),
    share()
)

export const hostInfoProcess = hostInfoRequest.pipe(
    exhaustMap(([email, token]) => from(hostServiceInfo(email, token))),
    share()
)

export const hostInfoSuccess = hostInfoProcess.pipe(
    filter((res) => hostServiceInfoValid(res))
)

export const hostInfoFailure = hostInfoProcess.pipe(
    filter((res) => hostServiceInfoError(res))
)

export const hostInfoLoading = loadingFrom(
    hostInfoRequest,
    hostInfoSuccess,
    hostInfoFailure
)

export const hostConfigRequest = hostInfoSuccess.pipe(
    map((res) => [
        hostServiceInfoIP(res),
        hostServiceInfoPort(res),
        hostServiceInfoSecret(res),
    ]),
    withLatestFrom(userEmail, userConfigToken),
    map(([[ip, port, secret], email, token]) => [
        ip,
        port,
        secret,
        email,
        token,
    ])
)

export const hostConfigProcess = hostConfigRequest.pipe(
    exhaustMap(([ip, port, secret, email, token]) =>
        hostServiceConfig(ip, port, secret, email, token)
    )
)
