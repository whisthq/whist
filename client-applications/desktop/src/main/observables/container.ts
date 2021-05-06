// This file is home to observables that manage container creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the containers while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success container creation emissions from here.

import { from, of, zip, combineLatest } from "rxjs"
import {
  map,
  share,
  delay,
  takeUntil,
  withLatestFrom,
  takeWhile,
} from "rxjs/operators"

import {
  containerCreate,
  containerCreateSuccess as createSuccess,
  containerCreateErrorNoAccess,
  containerCreateErrorUnauthorized,
  containerCreateErrorInternal,
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
import { containerPollingTimeout } from "@app/utils/constants"
import { pollMap, factory } from "@app/utils/observables"

import { formatContainer, formatTokensArray } from "@app/utils/formatters"
import { AsyncReturnType } from "@app/@types/state"

export const {
  request: containerCreateRequest,
  process: containerCreateProcess,
  success: containerCreateSuccess,
  failure: containerCreateFailure,
  loading: containerCreateLoading,
} = factory<[string, string], AsyncReturnType<typeof containerCreate>>(
  "containerCreate",
  {
    request: combineLatest([
      zip(userEmail, userAccessToken, userConfigToken),
      eventUpdateNotAvailable,
    ]).pipe(
      map(([auth]) => auth),
      map(([email, access, _]) => [email, access])
    ),
    process: ([email, token]) => from(containerCreate(email, token)),
    success: (res) => createSuccess(res),
    failure: (res) =>
      containerCreateErrorInternal(res) ||
      containerCreateErrorNoAccess(res) ||
      containerCreateErrorUnauthorized(res),
  }
)

export const {
  request: containerPollingRequest,
  process: containerPollingProcess,
  success: containerPollingSuccess,
  failure: containerPollingFailure,
  loading: containerPollingLoading,
} = factory<[string, string], AsyncReturnType<typeof containerInfo>>(
  "containerPolling",
  {
    request: containerCreateSuccess.pipe(
      withLatestFrom(userAccessToken),
      map(([response, token]) => [response.json.ID, token] as [string, string])
    ),
    process: (args) =>
      of(args).pipe(
        pollMap(1000, async ([id, token]) => await containerInfo(id, token)),
        takeWhile((res) => containerInfoPending(res), true),
        takeWhile((res) => !containerInfoError(res), true),
        takeUntil(of(true).pipe(delay(containerPollingTimeout))),
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
  }
)
