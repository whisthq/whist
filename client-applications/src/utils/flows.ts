/**
 * Copyright Fractal Computers, Inc. 2021
 * @file flows.ts
 * @brief This file contains utility functions for triggers/flows.
 */

import { Observable, ReplaySubject } from "rxjs"
import { filter, share, map, take } from "rxjs/operators"
import { mapValues, values } from "lodash"
import { withMocking } from "@app/testing"
// import { logBase, LogLevel } from "@app/utils/logging"
import TRIGGER from "@app/utils/triggers"

// A Trigger is emitted by an Observable. Every Trigger has a name and payload.
export interface Trigger {
  name: string
  payload: object | undefined
}

// This is the observable that emits Triggers.
export const TriggerChannel = new ReplaySubject<Trigger>()

export const fork = <T>(
  source: Observable<T>,
  filters: { [gate: string]: (result: T) => boolean }
): { [gate: string]: Observable<T> } => {
  /*
    Description:
      Takes in an observable and map of filter functions and outputs a map of filtered observables

    Arguments:
      source (Observable<T>): An observable whos emissions are desired to be filtered
      filters: Map of filter functions

    Returns:
      Map of filtered observables
  */

  const shared = source.pipe(share())
  return mapValues(filters, (fn) => shared.pipe(filter(fn), share()))
}

export const flow = <T>(
  name: string,
  flowFn: (t: Observable<T>) => { [key: string]: Observable<any> }
): ((t: Observable<T>) => { [key: string]: Observable<any> }) => {
  /*
    Description:
      A function that returns a function which, when a trigger is activated, will run
      and return a map of observables

    Arguments:
      name (string): Flow name
      fn: Function that takes in an observable and returns a map of observables

    Returns:
      Map of observables
  */
  return (trigger: Observable<T>) => {
    // Store the timestamp of when the flow starts, i.e. when the trigger fires
    let startTime = 0
    let triggerPayload: object | undefined = {}

    trigger.pipe(take(1)).subscribe((x?: any) => {
      startTime = Date.now() // Get the timestamp of when the flow started running
      triggerPayload = x // Save the trigger payload for logging down below
    })

    return mapValues(withMocking(name, trigger, flowFn), (obs, key) => {
      obs.subscribe((value: object) => {
        // logBase(
        //   `${name}.${key}`, // e.g. authFlow.success
        //   { input: triggerPayload, output: value }, // Log both the flow input (trigger) and output
        //   LogLevel.DEBUG,
        //   Date.now() - startTime // This is how long the flow took run
        // ).catch((err) => console.log(err))
      })

      return obs.pipe(share())
    })
  }
}

const triggerLogsBlacklist = ["networkUnstable"]

export const createTrigger = <A>(name: string, obs: Observable<A>) => {
  /*
      Description:
        Creates a Trigger from an observable

      Arguments:
        name (string): Trigger name
        obs (Observable<A>): Observable to be turned into a Trigger

      Returns:
        Original observable
    */

  const startTime = Date.now()
  obs.pipe(share()).subscribe((x: any) => {
    if (!triggerLogsBlacklist.includes(name)) {
      // logBase(
      //   `${name}`,
      //   { payload: x },
      //   LogLevel.DEBUG,
      //   Date.now() - startTime
      // ).catch((err) => console.log(err))
    }

    TriggerChannel.next({
      name: `${name}`,
      payload: x,
    })
  })

  return obs
}

export const fromTrigger = (name: string): Observable<any> => {
  /*
    Description:
      Returns a Trigger by name

    Arguments:
      name (string): Trigger name

    Returns:
      Observable
  */

  if (!values(TRIGGER).includes(name))
    throw new Error(`Trigger ${name} does not exist`)

  return TriggerChannel.pipe(
    // Filter out triggers by name. Note this allows for partial, case-insensitive string matching,
    // so filtering for "failure" will emit every time any trigger with "failure" in the name fires.
    filter((x: Trigger) => x.name === name),
    // Flatten the trigger so that it can be consumed by a subscriber without transforms
    map((x: Trigger) => x.payload)
  )
}
