// This file implements a wrapper that enables mocking the return values
// for flows. It checks an environment variable named TEST_MANUAL_SCHEMAS
// for the names of schemas that describe the new retun values.
//
// Mocking is enabled automatically on flows whose names exist in the
// selected schemas. Mocked flows will be triggered as they would in
// normal use of the application, but their return values will be the
// value described in the schema.
//
// Schemas are stored in src/testing/schemas as default-exported objects.
// Here, we import from src/testing/schemas/index.ts, so don't forget to
// first import your schema file there when add new schema files.
import { Observable, NEVER } from "rxjs"
import { get, set, keys, isEmpty, negate } from "lodash"
import * as schemas from "@app/testing/schemas"

// Arguments are passed through environment varialbes as positional arguments
// separated by commas processed in scripts/testManual.js
const schemaArguments = (process.env.TEST_MANUAL_SCHEMAS ?? "")
  .split(",")
  .filter(negate(isEmpty))

const testingEnabled = !isEmpty(schemaArguments)

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
if (testingEnabled) {
  console.log("TESTING MODE: Available schemas:", keys(schemas).join(", "))
  console.log("TESTING MODE: Selected schemas:", schemaArguments.join(", "))

  const available = new Set(keys(schemas))
  schemaArguments.forEach((arg) => {
    if (!available.has(arg))
      console.log("TESTING MODE: Received unknown schema argument:", arg)
  })
}

const getMocks = () => {
  return schemaArguments.reduce((result, value) => {
    const schema = get(schemas, value) ?? undefined
    if (schema !== undefined) return { ...result, ...schema }
    return result
  }, {})
}

// Create a map of EMPTY observables that has the
// specified keys.
const emptyFlow = (keys: string[]) =>
  keys.reduce((result, key) => {
    set(result as object, key, NEVER)
    return result
  }, {})

export const withMocking = <
  T extends Observable<any>,
  U extends { [key: string]: Observable<any> }
>(
  /*
    Hijacks flow observables with mock observables if running on test mode
    Arguments:
      name (str): name of the flow, ex. authFlow
      trigger (obs): observable trigger to mock
      channels (obj): map of key-observable pairs to replace with mock schema
  */
  name: string,
  trigger: T,
  flowFn: (trigger: T | typeof NEVER) => U
): { [P in keyof U]: Observable<any> } => {
  // Return early if we're not in testing mode.
  if (!testingEnabled) return flowFn(trigger)
  // Search the map of mockFns for the name of this flow
  // Return channels unchanged if not found
  const mockFn = get(getMocks(), name) ?? undefined

  if (mockFn === undefined) return flowFn(trigger)

  // We want any channels that are not defined in the DebugSchema to be
  // empty observables, so we don't cause errors for subscribers.
  return {
    ...emptyFlow(keys(flowFn(NEVER))),
    ...mockFn(trigger),
  }
}
