// Build the protocol, build tailwind, and run `snowpack dev` with
// test positional arguments. Extra arguments are added as a comma separated list
// and passed in as an environment variable.
//
// Ex: yarn build:manual loginError containerPollingError
//

const helpers = require("./build-package-helpers")

const args = process.argv.slice(2)

const withTesting = args.reduce((result, value) => {
  if (result.length === 0) return `${value}`
  return `${result}, ${value}`
}, "")

helpers.buildAndCopyProtocol()
helpers.buildTailwind()
helpers.snowpackDev({
  VERSION: helpers.getCurrentClientAppVersion(),
  WITH_MOCKS: withTesting,
  MANUAL_TEST: "true",
})
