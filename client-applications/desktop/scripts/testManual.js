// Build the protocol, build tailwind, and run `snowpack dev` with
// test positional arguments. Extra arguments are added as a comma separated list
// and passed in as an environment variable.
//
// Ex: yarn test:manual loginError mandelboxPollingError
//

const helpers = require("./build-package-helpers")
const { isEmpty } = require("lodash")
const path = require("path")

const args = process.argv.slice(2)

const schemaNames = args.reduce((result, value) => {
  if (result.length === 0) return `${value}`
  return `${result}, ${value}`
}, "")

if (isEmpty(schemaNames)) {
  const file = path.basename(process.argv[1])
  const message = `Schema names must be passed as arguments to ${file}`
  throw new Error(message)
}

helpers.buildAndCopyProtocol()
helpers.buildTailwind()
helpers.snowpackDev({
  VERSION: helpers.getCurrentClientAppVersion(),
  TEST_MANUAL_SCHEMAS: schemaNames,
})
