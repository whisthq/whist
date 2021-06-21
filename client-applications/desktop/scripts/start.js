// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers")

export default function start() {
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()
  helpers.snowpackDev({ VERSION: helpers.getCurrentClientAppVersion() })
}

if (require.main === module) {
  start()
}
