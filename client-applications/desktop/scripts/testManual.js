// Build the protocol, build tailwind, and run `snowpack dev`

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
