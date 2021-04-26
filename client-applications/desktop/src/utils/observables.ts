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

export const emitJSON = (
  observable: Observable<any>,
  str: string
) => observable.pipe(map((val) => ({ [str]: val })))

export const objectCombine = (obj: SubscriptionMap) =>
  merge(...toPairs(obj).map(([name, obs]) => emitJSON(obs, name)))
