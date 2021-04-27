import { Observable, ObservableInput, merge, race, interval, from } from "rxjs"
import { map, mapTo, switchMap } from "rxjs/operators"
import { toPairs } from "lodash"

export const loadingFrom = (
  request: Observable<any>,
  ...ends: Array<Observable<any>>
) =>
  merge(
    request.pipe(mapTo(true)),
    race(...ends.map((o) => o.pipe(mapTo(false))))
  )

export const pollMap = <T>(
  step: number,
  func: (...args: any[]) => ObservableInput<T>
) => {
  return switchMap((args) =>
    interval(step).pipe(switchMap(() => from(func(args))))
  )
}

export interface SubscriptionMap {
  [key: string]: Observable<any>
}

export const emitJSON = (observable: Observable<any>, str: string) =>
  /*
        Description:
            Used to map observables to objects so that observables can be "labeled".

            Takes in an observable and a string and returns an observable that emits a JSON where
            the key is the string and the value is the original observable

        Usage:
            const obs = interval(3000)

            emitJSON(obs, "interval").pipe(
              tap(x => console.log(x))
            )

            // Output: {"interval": 0}

        Arguments:
            observable (Observable<any>): The observable
            str (string): The name of the observable
    */

  observable.pipe(map((val) => ({ [str]: val })))

export const objectCombine = (obj: SubscriptionMap) =>
  /*
        Description:
            Takes in a SubscriptionMap and emits a key/value pair anytime any observable
            inthe SubscriptionMap emits

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
            obj (SubscriptionMap): Key/value mapping of observable names to observables
    */

  merge(...toPairs(obj).map(([name, obs]) => emitJSON(obs, name)))

export const objectFilter = (filterKey: string | undefined) =>
  /*
        Description:
            Custom operator which takes in an object and emits the value of a specified key

        Usage:
            const obs = emitJSON(interval(1000), "interval")
            obs.pipe(objectFilter("interval"), tap(
              x => console.log(x)
            ))

            // Output: 0 1 2 3

        Arguments:
            filterKey (string | undefined): The key to filter by. If undefined, the original
            object will be returned
    */

  map((obj: any) =>
    filterKey !== undefined && obj !== null ? obj[filterKey] : obj
  )
