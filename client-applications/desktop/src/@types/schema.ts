import { Observable } from "rxjs"

// We create a mock schema state by hijacking the mapValues iterator
// in flows.ts. We set the outer keys to be the name of the flow we want
// to hijack, ex. authFlow

export interface MockSchema {
    [key: string]: (trigger: Observable<any>) => {
        [key: string]: Observable<any>
    }
}
