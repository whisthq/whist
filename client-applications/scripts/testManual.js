// Build the protocol, build tailwind, and run `snowpack dev` with
// test positional arguments. Extra arguments are added as a comma separated list
// and passed in as an environment variable.
//
// Ex: yarn test:manual loginError mandelboxPollingError
//
// The positional arguments, e.g. loginError mandelboxPollingError, are the names
// of "schemas" defined in src/testing/schemas. These schemas describe "mock"
// behavior for flows in the main process. By passing the names of schemas to
// yarn test:manual, we force certain return values in client app flows so we
// can monitor how the application reacts.
//
// The implementation for this mocking is in src/testing/index.ts.

const helpers = require("./build-package-helpers")
const { isEmpty } = require("lodash")
const path = require("path")
const start = require("./start")

const testManual = (_env, ...args) => {
  const schemaNames = args.reduce((result, value) => {
    if (result.length === 0) return `${value}`
    return `${result}, ${value}`
  }, "")

  if (isEmpty(schemaNames)) {
    const file = path.basename(process.argv[1])
    const message = `Schema names must be passed as arguments to ${file}`
    throw new Error(message)
  }
  start({
    VERSION: helpers.getCurrentClientAppVersion(),
    TEST_MANUAL_SCHEMAS: schemaNames,
  })
}

module.exports = testManual

if (require.main === module) {
  testManual({}, ...process.argv.slice(2))
}
