import { Observable } from "rxjs"
import { map, filter, share, tap } from "rxjs/operators"
import { mapValues } from "lodash"

const gateSplit = <T>(
  source: Observable<T>,
  filters: { [gate: string]: (result: T) => boolean }
) => {
  const shared = source.pipe(share())
  return mapValues(filters, (fn) => shared.pipe(filter(fn)))
}
const gateMap = <A, B>(
  source: { [gate: string]: Observable<A> },
  fn: (x: A, s: string) => B
) => mapValues(source, (gate, key) => gate.pipe(map((value) => fn(value, key))))

export interface Gate<T> {
  [gate: string]: Observable<T>
}

export interface Flow<T = any> {
  (name: string, trigger: Observable<T>): Gate<T>
}

export const gates = <T>(
  name: string,
  trigger: Observable<T>,
  filters: { [channel: string]: (value: T) => boolean },
  logging?: { [channel: string]: ((value: T) => any) | undefined }
): Gate<T> =>
  mapValues(gateSplit(trigger, filters), (obs, key) =>
    obs.pipe(
      tap((value) =>
        console.log(
          `DEBUG: ${name}.${key} -- ${logging?.[key]?.(value) ?? value}`
        )
      ),
      share()
    )
  )
