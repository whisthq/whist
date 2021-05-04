// This file is home to observables that manage container creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the containers while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success container creation emissions from here.

import {
  containerCreate,
  containerInfo,
  containerInfoError,
  containerInfoSuccess,
  containerInfoPending,
} from "@app/utils/container"
import {
  userEmail,
  userAccessToken,
  userConfigToken,
} from "@app/main/observables/user"
import { eventUpdateNotAvailable } from "@app/main/events/autoupdate"
import { ContainerAssignTimeout } from "@app/utils/constants"
import { loadingFrom, pollMap } from "@app/utils/observables"
import { from, of, zip, combineLatest } from "rxjs"
import {
  map,
  share,
  delay,
  filter,
  takeUntil,
  exhaustMap,
  withLatestFrom,
  takeWhile,
} from "rxjs/operators"
import { formatContainer, formatTokensArray } from "@app/utils/formatters"
import { factory } from "@app/utils/observables"

export const containerCreateRequest = combineLatest([
  zip(userEmail, userAccessToken, userConfigToken),
  eventUpdateNotAvailable,
]).pipe(
  map(([auth]) => auth),
  map(([email, access, _]) => [email, access])
)

export const containerCreateProcess = containerCreateRequest.pipe(
  exhaustMap(([email, token]) => from(containerCreate(email, token))),
  share()
)

export const containerCreateSuccess = containerCreateProcess.pipe(
  filter((req) => (req?.json?.ID ?? "") !== "")
)

export const containerCreateFailure = containerCreateProcess.pipe(
  filter((req) => (req?.json?.ID ?? "") === "")
)

export const containerCreateLoading = loadingFrom(
  containerCreateRequest,
  containerCreateSuccess,
  containerCreateFailure
)

export const {
  request: containerAssignRequest,
  process: containerAssignProcess,
  success: containerAssignSuccess,
  failure: containerAssignFailure,
  loading: containerAssignLoading,
} = factory("containerAssign", {
  request: containerCreateSuccess.pipe(
    withLatestFrom(userAccessToken),
    map(([response, token]) => [response.json.ID, token] as [string, string])
  ),
  process: (args) =>
    of(args).pipe(
      pollMap(1000, async ([id, token]) => await containerInfo(id, token)),
      takeWhile((res) => containerInfoPending(res), true),
      takeWhile((res) => !containerInfoError(res), true),
      takeUntil(of(true).pipe(delay(ContainerAssignTimeout))),
      share()
    ),
  success: (res) => containerInfoSuccess(res),
  failure: (res) =>
    containerInfoError(res) ||
    containerInfoPending(res) ||
    !containerInfoSuccess(res),
  logging: {
    request: ["only tokens", formatTokensArray],
    process: ["omitting verbose response", formatContainer],
    success: ["omitting verbose response", formatContainer],
    failure: ["omitting verbose response", formatContainer],
  },
})
