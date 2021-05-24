import {
  Observable,
  ObservableInput,
  merge,
  race,
  interval,
  from,
  EMPTY,
  combineLatest,
} from "rxjs"
import {
  map,
  mapTo,
  switchMap,
  filter,
  takeLast,
  share,
  take,
} from "rxjs/operators"
import { toPairs, identity } from "lodash"

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

export const factory = <T, A>(
  /*
        Description:
            Accepts a string as the name of the factory, and a map of "stages".
            Outputs a map of observables that represent the different stages of
            a procedure.  Only the "request" and "process" keys are required.
            The map of "stages" has the following shape:
            {
              request: Observable that subscribes to an event source or another
                       observable. The "request" observable may filter or
                       throttle the subscription, thereby controlling whether an
                       event is processed by any of the next stages. The
                       "request" observable should return the exact arguments
                       needed by the "process" function.

              process: A function that receives the output of "request" and
                       returns an Observable. This represents the actual "work"
                       of this procedure. This is often something asynchronous
                       like an API call. The observable returned by the
                       "process" function must "complete", as only its final
                       emission will be used to determine "success" or "failure".

              success: A function that receives the final emission of "process",
                       returning a bool. This is a "test" of whether the output
                       of "process" was successful. If this test returns "true",
                       neither "failure" or "warning" should return "true".

              failure: A function that receives the final emission of "process",
                       returning a bool. This is a "test" of whether the output
                       of "process" has failed. If this test returns "true",
                       neither "success" or "warning" should return "true".

              warning: A function that receives the final emission of "process",
                       returning a bool. This is a "test" of whether the output
                       of "process" has neither succeeded nor failed. If this
                       test returns "true", neither "success" or "warning"
                       should return "true".

              logging: A map of "transformers" to adjust logging output. The
                       messageString should describe the transformation being
                       done to the logging output. func will take data from one
                       of the "stages" above and retun the output to be logged.
                       By default, the data from each stage will be logged
                       untransformed. The map has the following shape, with each
                       key being optional:
                      {
                        request: [messageString, func]
                        process: [messageString, func]
                        success: [messageString, func]
                        failure: [messageString, func]
                        warning: [messageString, func]
                        loading: [messageString, func]
                      }

            }

        Arguments:
            name (string): The name of the observable factory, used for logging.
            fx (dictionary): The map of "stages", described above.

        Returns:
            A map of observables with the following shape:
            {
                request: The observable passed to fx.request.
                process: The observable returned by the fx.process function.
                success: An observable emitting the final emission of fx.process,
                         only if fx.success returns true.
                failure: An observable emitting the final emission of fx.process,
                         only if fx.failure returns true.
                warning: An observable emitting the final emission of fx.process,
                         only if fx.warning returns true.
                loading: An observable that emits true when rx.request emits,
                         and false when either failure/warning/success emits.
            }
    */
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
  // We make the distinction between "processOuter" and "process" here, because
  // the fx.process function returns an observable that may emit more than once.
  // This is useful for "polling" functionality, where we need to perform a
  // process more than once to make sure it was successful.
  //
  // This is why the observable returned by "fx.process" must "complete", because
  // only its final value will be passed to the success/failure/warning funcs.
  //
  // We want to make use of each value emitted by fx.process's observable (for
  // the "process" logging), as well as the final value emitted by fx.process's
  // observable (for the "success/failure/warning" functions). We need two
  // separate pipes of the "fx.process" observable to accomplish this, so we
  // keep it in a higher-order observable ("processOuter") so we can reference
  // it more than once.

  const request = fx.request.pipe(share())
  const processOuter = request.pipe(map(fx.process), share())
  const process = processOuter.pipe(switchMap(identity), share())
  const results = processOuter.pipe(
    switchMap((o: Observable<A>) => o.pipe(takeLast(1), share()))
  )

  // The caller doesn't need to pass completion states they don't care about,
  // so if not passed then we just assign an empty observable (which completes
  // immediately and never emits.)
  const success = fx.success != null ? results.pipe(filter(fx.success)) : EMPTY
  const failure = fx.failure != null ? results.pipe(filter(fx.failure)) : EMPTY
  const warning = fx.warning != null ? results.pipe(filter(fx.warning)) : EMPTY

  // We can infer the loading state from the emissions of request + the
  // "completion" states (success, failure, warning).
  const loading = loadingFrom(request, success, failure, warning)

  return {
    request,
    process,
    success,
    failure,
    warning,
    loading,
  }
}

// export const objectSplit = <T extends Record<any, Observable<any>>>(
//   source: Observable<T>
// ) => {
//   const cache: { [P in keyof T]?: Subject<ObservedValueOf<T[P]>> } = {}
//   const cacheInit = (key: keyof T) => {
//     if (!(key in cache)) cache[key] = new Subject<ObservedValueOf<T[keyof T]>>()
//   }
//   const handler = {
//     get: (
//       target: { [P in keyof T]: Observable<ObservedValueOf<T[P]>> },
//       key: keyof T
//     ) => {
//       if (!(key in target)) {
//         cacheInit(key)
//         target[key] = cache[key]!.asObservable()
//       }
//       return target[key]
//     },
//   }

//   source.subscribe((map: any) => {
//     Object.entries(map).forEach(([key, obs]) => {
//       obs.subscribe((value: any) => {
//         cacheInit(key)
//         cache[key]!.next(value)
//       })
//     })
//   })

//   return new Proxy(
//     {} as { [P in keyof T]: Observable<ObservedValueOf<T[P]>> },
//     handler
//   )
// }

export const fromSignal = (obs: Observable<any>, signal: Observable<any>) =>
  combineLatest(obs, signal.pipe(take(1))).pipe(map(([x]) => x))
