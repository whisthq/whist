import { Observable, BehaviorSubject } from "rxjs"

// We create a mock schema state by hijacking the mapValues iterator
// in flows.ts. We set the outer keys to be the name of the flow we want
// to hijack, ex. loginFlow, signupFlow, etc.

export type FlowEffect = <
  C extends BehaviorSubject<{ [key: string]: any }>,
  T extends Observable<any>,
  U extends { [key: string]: Observable<any> }
>(
  context: C,
  name: string,
  trigger: T,
  channels: U
) => { [key: string]: Observable<any> }

export interface MockSchema {
  [key: string]: (trigger: Observable<any>) => {
    [key: string]: Observable<any>
  }
}
