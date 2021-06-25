// This file is the central import point for all our testing schema files.
// Each schema file should default export an object, and the keys in the object
// should be the names of observable flows defined with the 'flow' function.
// The value of each key is a function (trigger) => { channels }, the same
// type of function that is passed when defining a flow. For example, this
// object might be exported from loginError.ts:
//
//  export default {
//   authFlow: (trigger) => ({
//     failure: trigger.pipe(
//       delay(2000),
//       mapTo({ status: 400, json: { access_token: "" } }),
//       tap(() => console.log("MOCKED FAILURE"))
//     ),
//   }),
// }
//
// This object has a key named 'authFlow', which returns (trigger) => { failure }.
// Notice that the value of failure is an observable which fires 2000 ms after
// the trigger.
//
// This "mock flow" will be enabled if the "schema name" (loginError), is present
// in the environment variable TEST_MANUAL_SCHEMAS. If enabled, then when a flow
// named "authFlow" is triggered, this "mock authFlow" will fire instead of the
// real flow. This "mock authFlow" guarantees that a failure event will occur
// 2000ms after the authFlow is triggered, so we can see how the rest of the
// application will react to the authFlow failure.
export { default as loginError } from "./loginError"
export { default as mandelboxCreateError } from "./mandelboxCreateError"
