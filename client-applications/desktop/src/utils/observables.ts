import { Observable, merge, race } from "rxjs"
import { mapTo, startWith } from "rxjs/operators"

export const loadingFrom = (
    request: Observable<any>,
    success: Observable<any>,
    failure: Observable<any>
) =>
    merge(
        request.pipe(mapTo(true), startWith(false)),
        race(success.pipe(mapTo(false)), failure.pipe(mapTo(false)))
    )
