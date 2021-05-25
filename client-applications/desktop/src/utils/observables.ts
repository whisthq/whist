import {
  Observable,
  ObservableInput,
  merge,
  race,
  interval,
  from,
  combineLatest,
} from "rxjs"
import { map, mapTo, switchMap, take } from "rxjs/operators"
import { toPairs, fromPairs, isFunction } from "lodash"

export const loadingFrom = (
  /*
        Description:
            A helper that automates the creation of a "loading" observable.

        Arguments:
            request: An observable whose emissions represent the "start" of a
                     loading process.
            ...ends: A list of observables whose emissions represent the "end"
                     of a loading process.
        Returns:
            An observable that emits "true" when "request" emits, and "false"
            as soon as any of the "...ends" observables emit. Only one "end"
            observable should emit at a time, any subsequent "end" emissions
            will be ignored until "request" emits again.
    */
  request: Observable<any>,
  ...ends: Array<Observable<any>>
) =>
  merge(
    request.pipe(mapTo(true)),
    race(...ends.map((o) => o.pipe(mapTo(false))))
  )

export const pollMap = <T>(
  /*
        Description:
            An adorable little helper to perform a function on a time interval.

        Arguments:
            step: A number of milliseconds to wait between each function call.
            func: A function that returns a value, a Promise, or an
                  observable.
        Returns:
            An observable that emits the result of func at every time step. The
            observable will emit infinitely, it's up to the caller to
            unsubscribe from it. The returned observable will wait for the first
            time step before emitting its first value.
    */
  step: number,
  func: (...args: any[]) => ObservableInput<T>
) => {
  return switchMap((args) =>
    interval(step).pipe(switchMap(() => from(func(args))))
  )
}

export const objectCombine = <T extends { [key: string]: Observable<any> }>(
  obj: T
) =>
  /*
        Description:
            Takes in a map of observable values and emits a key/value pair
            anytime any observable in the map emits.

        Usage:
            const sMap = {
              a: interval(1000),
              b: interval(2000)
            }

            objectCombine(sMap).pipe(
              tap(x => console.log(x))
            )

            // Output: {"a": 0} {"b": 1}

        Arguments:
            obj: Key/value mapping of observable names to observables
    */

  merge(
    ...toPairs(obj).map(([name, obs]) =>
      obs.pipe(map((val) => ({ [name]: val })))
    )
  )

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

// This doesn't have anything to do with observables, but it's too useful to
// delete.
const pickMap = <
  T extends Record<string, any>,
  K extends { [P in keyof T]?: ((v: T[P]) => any) | K }
>(
  /*
    Description:
      Recursively flattens json objects into a single level of key-value pairs

    Arguments:
      obj: A Record of type String, any to be spreaded to a new object
      fmap: Map of functions to apply to key value pairs in obj

    Returns:
      JSON object from flattend obj and fmap
  */
  obj: T,
  fmap: K
): T => ({
  ...obj,
  ...fromPairs(
    toPairs(fmap).map(([key, value]) => [
      key,
      isFunction(value) ? value(obj[key]) : pickMap(obj[key], value),
    ])
  ),
})
