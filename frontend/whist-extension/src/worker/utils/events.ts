import { ReplaySubject } from "rxjs"
import { filter, map, share } from "rxjs/operators"

const WhistEvent = new ReplaySubject()

const createEvent = (name: string, payload?: object) => {
  WhistEvent.next({
    name,
    ...(payload !== undefined && { payload }),
  })
}

const fromEvent = (name: string) => {
  return WhistEvent.pipe(
    // Filter out triggers by name. Note this allows for partial, case-insensitive string matching,
    // so filtering for "failure" will emit every time any trigger with "failure" in the name fires.
    filter((x: { name: string; payload: object }) => x.name === name),
    // Flatten the trigger so that it can be consumed by a subscriber without transforms
    map((x: { name: string; payload: object }) => x.payload),
    share()
  )
}

export { createEvent, fromEvent }
