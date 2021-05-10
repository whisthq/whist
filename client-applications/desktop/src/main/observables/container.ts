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
import { containerPollingTimeout } from "@app/utils/constants"
import { loadingFrom, pollMap } from "@app/utils/observables"
import { from, of, zip, combineLatest } from "rxjs"
import {
  map,
  delay,
  takeUntil,
  withLatestFrom,
  takeWhile,
  switchMap,
} from "rxjs/operators"
import { gates } from "@app/utils/gates"

export const {
  success: containerCreateSuccess,
  failure: containerCreateFailure,
  warning: containerCreateWarning,
} = gates(
  `containerCreate`,
  combineLatest([
    zip(userEmail, userAccessToken, userConfigToken),
    eventUpdateNotAvailable,
  ]).pipe(
    map(([auth]) => auth), //discard the eventUpdateNotAvailable
    switchMap(([email, token]) => from(containerCreate(email, token)))
  ),
  {
    success: (req: { json: { ID: string } }) => (req?.json?.ID ?? "") !== "",
    failure: (req: { json: { ID: string } }) => (req?.json?.ID ?? "") === "",
  }
)

export const {
  success: containerPollingSuccess,
  failure: containerPollingFailure,
} = gates(
  `containerPolling`,
  containerCreateSuccess.pipe(
    withLatestFrom(userAccessToken),
    pollMap(
      1000,
      async ([response, token]) => await containerInfo(response.json.ID, token)
    ),
    takeWhile((res) => containerInfoPending(res), true),
    takeWhile((res) => !containerInfoError(res), true),
    takeUntil(of(true).pipe(delay(containerPollingTimeout)))
  ),
  {
    success: (res) => containerInfoSuccess(res),
    failure: (res) =>
      containerInfoError(res) ||
      containerInfoPending(res) ||
      !containerInfoSuccess(res),
  }
)

export const containerPollingLoading = loadingFrom(
  containerCreateSuccess.pipe(withLatestFrom(userAccessToken)),
  containerPollingSuccess,
  containerPollingFailure
)
