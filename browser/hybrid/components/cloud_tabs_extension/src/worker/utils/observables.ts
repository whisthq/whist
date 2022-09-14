import { map, withLatestFrom } from "rxjs/operators"

const waitFor = (obs: any, waitFor: any) =>
  obs.pipe(
    withLatestFrom(waitFor),
    map(([obs]) => obs)
  )

export { waitFor }
