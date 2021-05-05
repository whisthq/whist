import {
  Observable,
  ObservableInput,
  merge,
  race,
  interval,
  from,
  EMPTY,
} from "rxjs"
import { map, mapTo, switchMap, filter, takeLast } from "rxjs/operators"
import { toPairs, compact, identity, isEmpty } from "lodash"
import { debugObservables } from "@app/utils/logging"

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

export const factory = <T, A>(
  name: string,
  fx: {
    request: Observable<T>
    process: (ax: T) => Observable<A>
    success?: (dx: A) => boolean
    failure?: (dx: A) => boolean
    warning?: (dx: A) => boolean
    logging?: {
      request?: [string, (_: T) => any]
      process?: [string, (_: A) => any]
      success?: [string, (_: A) => any]
      failure?: [string, (_: A) => any]
      warning?: [string, (_: A) => any]
      loading?: [string, (_: boolean) => any]
    }
  }
) => {
  const request = fx.request
  const processOuter = request.pipe(map(fx.process))
  const process = processOuter.pipe(switchMap(identity))
  const results = processOuter.pipe(switchMap((obs) => obs.pipe(takeLast(1))))

  const success = fx.success != null ? results.pipe(filter(fx.success)) : EMPTY
  const failure = fx.failure != null ? results.pipe(filter(fx.failure)) : EMPTY
  const warning = fx.warning != null ? results.pipe(filter(fx.warning)) : EMPTY

  const loading = loadingFrom(request, ...compact([success, failure, warning]))

  const logging = [
    ["Request", request, ...(fx?.logging?.request ?? [])],
    ["Process", process, ...(fx?.logging?.process ?? [])],
    ["Success", success, ...(fx?.logging?.success ?? [])],
    ["Failure", failure, ...(fx?.logging?.failure ?? [])],
    ["Warning", warning, ...(fx?.logging?.warning ?? [])],
    ["Loading", loading, ...(fx?.logging?.loading ?? [])],
  ] as Array<[string, Observable<any>?, string?, ((...args: any[]) => any)?]>

  debugObservables(
    ...logging.map(
      ([stage, obs, message = "", transform]) =>
        [
          obs?.pipe?.(map(transform ?? identity)) ?? EMPTY,
          `${name}${stage}${isEmpty(message) ? message : " -- "}${message}`,
        ] as [Observable<any>, string]
    )
  )

  return {
    request,
    process,
    success,
    failure,
    warning,
    loading,
  }
}
