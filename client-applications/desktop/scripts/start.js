// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers")

helpers.buildAndCopyProtocol()
helpers.buildTailwind()
helpers.snowpackDev(helpers.getCurrentClientAppVersion())
