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
    containerInfoPending,
} from "@app/utils/container"
import { userEmail, userAccessToken } from "@app/main/observables/user"
import { ContainerAssignTimeout } from "@app/utils/constants"
import { eventAppReady } from "@app/main/events/app"
import { from, merge, of, combineLatest, interval } from "rxjs"
import {
    mapTo,
    map,
    last,
    share,
    delay,
    filter,
    takeUntil,
    exhaustMap,
    withLatestFrom,
    switchMap,
    takeWhile,
} from "rxjs/operators"


export const containerCreateRequest = combineLatest([
    eventAppReady,
    userAccessToken,
]).pipe(
    map(([_, token]) => token),
    withLatestFrom(userEmail),
    map(([token, email]) => containerCreate(email!, token!)),
    share()
)

export const containerCreateLoading = containerCreateRequest.pipe(
    exhaustMap((req) => merge(of(true), from(req).pipe(mapTo(false))))
)

export const containerCreateSuccess = containerCreateRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((req) => !containerCreateError(req))
)

export const containerCreateFailure = containerCreateRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((req) => containerCreateError(req))
)

export const containerAssignRequest = containerCreateSuccess.pipe(
    withLatestFrom(userAccessToken),
    map(([{ json }, token]) =>
        interval(1000).pipe(
            switchMap(() => containerInfo(json.ID, token!)),
            takeWhile((res) => containerInfoPending(res), true),
            takeWhile((res) => !containerInfoError(res), true),
            takeUntil(of(true).pipe(delay(ContainerAssignTimeout))),
            share()
        )
    )
)

export const containerAssignLoading = containerAssignRequest.pipe(
    exhaustMap((req) => merge(of(true), from(req).pipe(last(), mapTo(false))))
)

export const containerAssignPolling = containerAssignRequest.pipe(
    exhaustMap((req) => from(req).pipe(map((res: any) => res.json?.state)))
)

export const containerAssignSuccess = containerAssignRequest.pipe(
    exhaustMap((req) => from(req).pipe(last())),
    filter((req) => containerInfoSuccess(req))
)

export const containerAssignFailure = containerAssignRequest.pipe(
    exhaustMap((req) => from(req).pipe(last())),
    filter(
        (req) =>
            containerInfoError(req) ||
            containerInfoPending(req) ||
            !containerInfoSuccess(req)
    )
)
