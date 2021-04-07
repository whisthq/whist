// This file is home to observables that manage container creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the containers while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success container creation emissions from here.

import {
  containerCreate,
  containerCreateError,
  containerInfo,
  containerInfoError,
  containerInfoSuccess,
  containerInfoPending
} from '@app/utils/container'
import { userEmail, userAccessToken } from '@app/main/observables/user'
import { loginSuccess } from '@app/main/observables/login'
import { ContainerAssignTimeout } from '@app/utils/constants'
import { loadingFrom } from '@app/utils/observables'
import { from, of, interval, merge } from 'rxjs'
import {
  map,
  last,
  share,
  delay,
  filter,
  takeUntil,
  exhaustMap,
  withLatestFrom,
  switchMap,
  takeWhile
} from 'rxjs/operators'

export const containerCreateRequest = loginSuccess.pipe(
  withLatestFrom(userEmail, userAccessToken),
  map(([_, email, token]) => [email, token])
)

export const containerCreateProcess = containerCreateRequest.pipe(
  exhaustMap(([email, token]) => from(containerCreate(email, token))),
  share()
)

export const containerCreateSuccess = containerCreateProcess.pipe(
  filter((req) => !containerCreateError(req))
)

export const containerCreateFailure = containerCreateProcess.pipe(
  filter((req) => containerCreateError(req))
)

export const containerCreateLoading = loadingFrom(
  containerCreateRequest,
  containerCreateSuccess,
  containerCreateFailure
)

export const containerAssignRequest = containerCreateSuccess.pipe(
  withLatestFrom(userAccessToken),
  map(([response, token]) => [response.json.ID, token])
)

export const containerAssignPolling = containerAssignRequest.pipe(
  map(([id, token]) =>
    interval(1000).pipe(
      switchMap(() => from(containerInfo(id, token))),
      takeWhile((res) => containerInfoPending(res), true),
      takeWhile((res) => !containerInfoError(res), true),
      takeUntil(of(true).pipe(delay(ContainerAssignTimeout))),
      share()
    )
  )
)

containerAssignPolling
  .pipe(switchMap((res) => res))
  .subscribe((res) =>
    console.log('container poll', res?.status, res?.json.state)
  )

export const containerAssignSuccess = containerAssignPolling.pipe(
  exhaustMap((poll) => poll.pipe(last())),
  filter((res) => containerInfoSuccess(res))
)

export const containerAssignFailure = merge(
  containerAssignPolling.pipe(
    exhaustMap((poll) => poll.pipe(last())),
    filter(
      (res) =>
        containerInfoError(res) ||
                containerInfoPending(res) ||
                !containerInfoSuccess(res)
    )
  )
)

export const containerAssignLoading = loadingFrom(
  containerAssignRequest,
  containerAssignSuccess,
  containerAssignFailure
)
