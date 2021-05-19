import { Observable, EMPTY } from "rxjs"
import { get, set, keys } from "lodash"
import mocks from "@app/main/testing/state"

// export const mockState = (value: any, name: string, key: string) => {
//   const func = get(loginError, [name, key], of)
//   // console.log("NAME: ", name, "KEY ", key)
//   return func(value)
// }

// This test should check for an enviroment variable, or some indicator
// that we're in "testing mode". We'll just set to true for now.
const isMockingEnabled = () => true

// These arguments should probably be passed through the `yarn test:manual`
// command or however we enter testing mode.
// For now, we'll just return a static list of the loginError string.
const getMockArgs = () => ["loginError"]

// The form of this map will be something like:
// {
//    authFlow: {
//        success: Observable<any>,
//        failure: Observable<any>,
//    }
// }
// We only need to create this map once at program start, so the below
// variable holds a map that looks like the example above.
// The withMocking function will check this map for a name like "authFlow".
// If found, withMocking will use the value at "authFlow" as its output.
const mockFns = getMockArgs().reduce((result, value) => {
  if (get(mocks, value)) return { ...result, ...get(mocks, value) }
  return result
}, {})

// Create a map of EMPTY observables that has the
// specified keys.
const emptyFlow = (keys: string[]) =>
  keys.reduce((result, key) => {
    set(result as object, key, EMPTY)
    return result
  }, {})

export const withMocking = <
  T extends Observable<any>,
  U extends { [key: string]: Observable<any> }
>(
  name: string,
  trigger: T,
  channels: U
): { [P in keyof U]: Observable<any> } => {
  // Return early if we're not in testing mode.
  if (!isMockingEnabled()) return channels

  // Search the map of mockFns for the name of this flow
  // Return channels unchanged if not found
  const mockFn = get(mockFns, name)
  if (!mockFn) return channels

  // We want any channels that are not defined in the DebugSchema to be
  // empty observables, so we don't cause errors for subscribers.
  return {
    ...emptyFlow(keys(channels)),
    ...mockFn(trigger),
  }
}
