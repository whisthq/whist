import {
  userSub,
  userAccessToken,
  userConfigToken,
} from "@app/main/observables/user"
import {
  containerPollingFailure,
  containerPollingLoading,
} from "@app/main/observables/container"
import { loadingFrom, pollMap } from "@app/utils/observables"
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
} from "@app/utils/host"
import { debugObservables, errorObservables } from "@app/utils/logging"
import { from } from "rxjs"
import {
  map,
  share,
  filter,
  takeLast,
  takeWhile,
  takeUntil,
  exhaustMap,
  withLatestFrom,
} from "rxjs/operators"
import {
  formatHostConfig,
  formatHostInfo,
  formatObservable,
  formatTokensArray,
} from "@app/utils/formatters"

export const hostInfoRequest = containerPollingLoading.pipe(
  withLatestFrom(userSub, userAccessToken),
  map(([_, email, token]) => [email, token] as [string, string]),
  share()
)

export const hostInfoPolling = hostInfoRequest.pipe(
  pollMap(1000, async ([email, token]) => await hostServiceInfo(email, token)),
  takeWhile((res) => hostServiceInfoPending(res), true),
  takeUntil(containerPollingFailure),
  share()
)

hostInfoPolling.subscribe((res: any) =>
  console.log("host poll", res?.status, res?.json)
)

export const hostInfoSuccess = hostInfoPolling.pipe(
  takeLast(1),
  filter((res) => hostServiceInfoValid(res))
)

export const hostInfoFailure = hostInfoPolling.pipe(
  takeLast(1),
  filter((res) => hostServiceInfoPending(res) || !hostServiceInfoValid(res))
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
  withLatestFrom(userSub, userConfigToken),
  map(([[ip, port, secret], email, token]) => [ip, port, secret, email, token])
)

export const hostConfigProcess = hostConfigRequest.pipe(
  exhaustMap(([ip, port, secret, email, token]) =>
    from(hostServiceConfig(ip, port, secret, email, token))
  ),
  share()
)

export const hostConfigSuccess = hostConfigProcess.pipe(
  filter((res: { status: number }) => hostServiceConfigValid(res))
)

export const hostConfigFailure = hostConfigProcess.pipe(
  filter((res: { status: number }) => hostServiceConfigError(res))
)

export const hostConfigLoading = loadingFrom(
  hostInfoRequest,
  hostInfoSuccess,
  hostInfoFailure
)

// Logging

debugObservables(
  [
    formatObservable(hostInfoRequest, formatTokensArray),
    userSub,
    "hostInfoRequest",
  ],
  [
    formatObservable(hostInfoSuccess, formatHostInfo),
    userSub,
    "hostInfoSuccess",
  ],
  [hostConfigRequest, userSub, "hostConfigRequest"],
  [
    formatObservable(hostConfigProcess, formatHostConfig),
    userSub,
    "hostConfigProcess",
  ],
  [
    formatObservable(hostConfigSuccess, formatHostConfig),
    userSub,
    "hostConfigSuccess",
  ]
)

errorObservables(
  [hostInfoFailure, userSub, "hostInfoFailure"],
  [hostConfigFailure, userSub, "hostConfigFailure"]
)
