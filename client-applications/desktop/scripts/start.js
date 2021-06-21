// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers")

const start = (env, ..._args) => {
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()
  helpers.snowpackDev({ ...env, VERSION: helpers.getCurrentClientAppVersion() })
}

module.exports = start

if (require.main === module) {
  start()
}
