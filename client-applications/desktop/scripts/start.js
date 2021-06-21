// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers")

export default function start(env, ..._args) {
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()
  helpers.snowpackDev({ ...env, VERSION: helpers.getCurrentClientAppVersion() })
}

if (require.main === module) {
  start()
}
