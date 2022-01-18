/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file auth.ts
 * @brief This file contains custom RXJS operators that add functionality on top of existing RXJS
 * operators.
 */

import { Observable, combineLatest, zip } from "rxjs"
import { map, withLatestFrom, take, takeUntil } from "rxjs/operators"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

export const waitForSignal = (obs: Observable<any>, signal: Observable<any>) =>
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

export const emitOnSignal = (obs: Observable<any>, signal: Observable<any>) =>
  /*
        Description:
            Whereas waitForSignal will fire obs as many times as obs emits, emitOnSignal
            will fire obs once every time signal emits. If obs fires after signal,
            we wait for obs to fire and then emit once.

        Arguments:
            obs (Observable<any>): The observable that you want to fire
            signal (Observable<any>): The observable that you are waiting for to fire first
        Returns:
            An observable that fires only when the "signal" observable has fired.
    */
  zip(obs, signal).pipe(
    withLatestFrom(obs),
    map(([, obs]) => obs)
  )

export const withAppReady = (obs: Observable<any>) =>
  waitForSignal(obs, fromTrigger(WhistTrigger.appReady))

export const untilUpdateAvailable = (obs: Observable<any>) =>
  obs.pipe(takeUntil(fromTrigger(WhistTrigger.updateAvailable)))
