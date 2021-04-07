import { Observable, ObservableInput, merge, race, interval, from } from "rxjs"
import { mapTo, startWith, switchMap, share } from "rxjs/operators"

export const loadingFrom = (
    request: Observable<any>,
    ...ends: Observable<any>[]
) =>
    merge(
        request.pipe(mapTo(true), startWith(false)),
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
