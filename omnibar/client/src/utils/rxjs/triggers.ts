/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file defines all the triggers recognized by the main thread.
 */
import { Observable, ReplaySubject } from "rxjs";
import { filter, map, tap, share } from "rxjs/operators";
import { values } from "lodash";

import { localLog } from "@app/utils/logging";
import { FractalTrigger } from "@app/utils/constants/triggers";

// A Trigger is emitted by an Observable. Every Trigger has a name and payload.
export interface Trigger {
  name: FractalTrigger;
  payload: any;
}

// This is the observable that emits Triggers.
export const TriggerChannel = new ReplaySubject<Trigger>();

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

  obs.pipe(share()).subscribe((x: any) => {
    TriggerChannel.next({
      name: `${name}` as FractalTrigger,
      payload: x,
    });
  });

  return obs;
};

export const fromTrigger = (name: FractalTrigger): Observable<any> => {
  /*
    Description: 
      Returns a Trigger by name
    
    Arguments: 
      name (string): Trigger name

    Returns: 
      Observable
  */

  if (!values(FractalTrigger).includes(name))
    throw new Error(`Trigger ${name} does not exist`);

  return TriggerChannel.pipe(
    // Filter out triggers by name. Note this allows for partial, case-insensitive string matching,
    // so filtering for "failure" will emit every time any trigger with "failure" in the name fires.
    filter((x: Trigger) => x.name === name),
    tap((x) => localLog(x)),
    // Flatten the trigger so that it can be consumed by a subscriber without transforms
    map((x: Trigger) => x.payload)
  );
};
