import { Observable, ReplaySubject, BehaviorSubject, Subject } from "rxjs"
import { filter, share, map, scan } from "rxjs/operators"
import { mapValues, values } from "lodash"
import { withMocking } from "@app/main/testing"
import { withLogging } from "@app/main/logging"
import TRIGGER from "@app/main/triggers/constants"

// A Trigger is emitted by an Observable. Every Trigger has a name and payload.
export interface Trigger {
  name: string
  payload: any
}

// This is the observable that emits Triggers.
export const TriggerChannel = new ReplaySubject<Trigger>()

// Some rather messy state management. We shouldn't try and use this anywhere
// in the app, it's intended right now to help give some contextual information
// to FlowEffects like logging, which needs information like user email.
//
// We'll add things to the StateChannel in main/effects/state, and then
// we can getValue() from the state BehaviorSubject when we need it.
// The only consumer of this state should be the "flow" function.
const state = new BehaviorSubject<object>({})
export const StateChannel = new Subject<object>()
StateChannel.pipe(scan((acc, val) => ({ ...acc, ...val }))).subscribe((value) =>
  state.next(value)
)

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
  return mapValues(filters, (fn) => shared.pipe(filter(fn)))
}

export const flow =
  <T>(
    name: string,
    fn: (t: Observable<T>) => { [key: string]: Observable<any> }
  ): ((t: Observable<T>) => { [key: string]: Observable<any> }) =>
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

  (trigger: Observable<T>) => {
    const channels = fn(trigger)

    const mocked = withMocking(state, name, trigger, channels)
    const logged = withLogging(state, name, trigger, mocked)

    return mapValues(logged, (obs) => obs.pipe(share()))
  }

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

  obs.subscribe((x: A) => {
    TriggerChannel.next({ name: `${name}`, payload: x } as Trigger)
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
