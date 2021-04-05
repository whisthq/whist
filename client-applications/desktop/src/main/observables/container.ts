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
import { loadingFrom } from "@app/utils/observables"
import { from, merge, of, interval } from "rxjs"
import {
    mapTo,
    map,
    last,
    share,
    sample,
    delay,
    filter,
    takeUntil,
    exhaustMap,
    withLatestFrom,
    switchMap,
    takeWhile,
} from "rxjs/operators"

export const containerCreateRequest = eventAppReady.pipe(
    sample(userAccessToken),
    withLatestFrom(userEmail),
    map(([token, email]) => [email, token])
)

export const containerCreateProcess = containerCreateRequest.pipe(
    exhaustMap(([email, token]) => from(containerCreate(email!, token!))),
    share()
)

export const containerCreateSuccess = containerCreateProcess.pipe(
    filter((req) => !containerCreateError(req))
)

export const containerCreateFailure = containerCreateProcess.pipe(
    filter((req) => containerCreateError(req))
)

export const containerAssignRequest = containerCreateSuccess.pipe(
    withLatestFrom(userAccessToken),
    map(([response, token]) => [response.json.ID, token!])
)

export const containerAssignProcess = containerAssignRequest.pipe(
    exhaustMap(([id, token]) =>
        interval(1000).pipe(
            switchMap(() => containerInfo(id, token)),
            takeWhile((res) => containerInfoPending(res), true),
            takeWhile((res) => !containerInfoError(res), true),
            takeUntil(of(true).pipe(delay(ContainerAssignTimeout))),
            share()
        )
    )
)

export const containerAssignPolling = containerAssignRequest.pipe(
    exhaustMap((req) => from(req).pipe(map((res: any) => res.json?.state)))
)

export const containerAssignSuccess = containerAssignRequest.pipe(
    filter((req) => containerInfoSuccess(req))
)

export const containerAssignFailure = containerAssignRequest.pipe(
    filter(
        (req) =>
            containerInfoError(req) ||
            containerInfoPending(req) ||
            !containerInfoSuccess(req)
    )
)

export const containerAssignLoading = loadingFrom(
    containerAssignRequest,
    containerAssignSuccess,
    containerAssignFailure
)
