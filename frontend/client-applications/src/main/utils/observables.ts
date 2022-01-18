/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file auth.ts
 * @brief This file contains custom RXJS operators that add functionality on top of existing RXJS
 * operators.
 */

import { Observable, combineLatest } from "rxjs"
import { map, withLatestFrom, take, takeUntil } from "rxjs/operators"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

export const fromSignal = (obs: Observable<any>, signal: Observable<any>) =>
  /*
        Description:
            A helper that allows one observable to fire only when another observable
            has fired.

        Arguments:
            obs (Observable<any>): The observable that you want to fire
            signal (Observable<any>): The observable that you are waiting for to fire first
        Returns:
            An observable that fires only when the "signal" observable has fired.
    */
  combineLatest(obs, signal.pipe(take(1))).pipe(map(([x]) => x))

export const onSignal = (obs: Observable<any>, signal: Observable<any>) =>
  /*
        Description:
            A helper that allows one observable to fire only when another observable
            has fired.

        Arguments:
            obs (Observable<any>): The observable that you want to fire
            signal (Observable<any>): The observable that you are waiting for to fire first
        Returns:
            An observable that fires only when the "signal" observable has fired.
    */
  signal.pipe(
    withLatestFrom(obs),
    map(([, obs]) => obs)
  )

export const withAppReady = (obs: Observable<any>) =>
  fromSignal(obs, fromTrigger(WhistTrigger.appReady))

export const untilUpdateAvailable = (obs: Observable<any>) =>
  obs.pipe(takeUntil(fromTrigger(WhistTrigger.updateAvailable)))
