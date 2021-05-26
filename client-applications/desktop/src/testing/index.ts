import { Observable, EMPTY } from "rxjs"
import { get, set, keys } from "lodash"
import * as schemas from "@app/testing/schemas"

// This test should check for an enviroment variable, or some indicator
// that we're in "testing mode". We'll just set to true for now.
const isMockingEnabled = () => process.env.MANUAL_TEST === "true"

// Arguments are passed through environment varialbes as positional arguments
// separated by commas processed in scripts/testManual.js
const getMockArgs = () => process.env.WITH_MOCKS?.split(",") ?? []

// The form of this map will be something like:
// {
//    authFlow: {
//        success: Observable<any>,
//        failure: Observable<any>,
//    }
// }
// We only need to create this map once at program start, so the below
// function creates a map that looks like the example above.
// The withMocking function will check this map for a name like "authFlow".
// If found, withMocking will use the value at "authFlow" as its output.
// Just as a note, getMocks() will run with every invocation of flow(), albeit
// only in testing mode. Some optimization here can be desired eventually,
// such as only calling the mocks once instead of every time in withMocking()
// when a flow is created.
const getMocks = () => {
  const args = getMockArgs()
  return args.reduce((result, value) => {
    const schema = get(schemas, value) ?? undefined
    if (schema !== undefined) return { ...result, ...schema }
    return result
  }, {})
}

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
  /*
    Hijacks flow observables with mock observables if running on test mode
    Arguments: 
      name (str): name of the flow, ex. loginFlow, signupFlow, authFlow 
      trigger (obs): observable trigger to mock 
      channels (obj): map of key-observable pairs to replace with mock schema
  */
  name: string,
  trigger: T,
  channels: U
): { [P in keyof U]: Observable<any> } => {
  // Return early if we're not in testing mode.
  if (!isMockingEnabled()) return channels

  // Search the map of mockFns for the name of this flow
  // Return channels unchanged if not found
  const mockFn = get(getMocks(), name) ?? undefined
  if (mockFn === undefined) return channels

  // We want any channels that are not defined in the DebugSchema to be
  // empty observables, so we don't cause errors for subscribers.
  return {
    ...emptyFlow(keys(channels)),
    ...mockFn(trigger),
  }
}
