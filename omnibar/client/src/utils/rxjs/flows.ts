/**
 * Copyright Fractal Computers, Inc. 2021
 * @file flows.ts
 * @brief This file contains utility functions for triggers/flows.
 */

import { Observable } from "rxjs";
import { filter, share } from "rxjs/operators";
import { mapValues } from "lodash";

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

  const shared = source.pipe(share());
  return mapValues(filters, (fn) => shared.pipe(filter(fn), share()));
};

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
    return mapValues(flowFn(trigger), (obs) => obs.pipe(share()));
  };
};
