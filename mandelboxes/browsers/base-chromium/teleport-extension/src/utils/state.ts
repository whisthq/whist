import { BehaviorSubject } from "rxjs"
import { take } from "rxjs/operators"

const state = {
  overscrollX: new BehaviorSubject(0),
} as { [key: string]: BehaviorSubject<any> }

const setState = (name: string, payload: any) => {
  if (!Object.keys(state).includes(name))
    throw new Error(`State ${name} does not exist`)

  state[name].next(payload)
}

const getState = (name: string, callback: (x: any) => void) =>
  state[name].pipe(take(1)).subscribe((x) => callback(x))

export { setState, getState }
