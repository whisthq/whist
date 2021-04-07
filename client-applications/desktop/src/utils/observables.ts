import { Observable, merge, race } from "rxjs"
import { mapTo, startWith } from "rxjs/operators"

export const loadingFrom = (
    request: Observable<any>,
    ...ends: Observable<any>[]
) =>
    merge(
        request.pipe(mapTo(true), startWith(false)),
        race(...ends.map((o) => o.pipe(mapTo(false))))
    )
