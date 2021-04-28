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

  merge(
    ...toPairs(obj).map(([name, obs]) =>
      obs.pipe(map((val) => ({ [name]: val })))
    )
  )
