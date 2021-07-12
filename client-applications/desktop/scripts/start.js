// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const start = (env, ..._args) => {
  helpers.removeEnvOverridesFile()

  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()

  helpers.snowpackDev({ ...env, VERSION: helpers.getCurrentClientAppVersion() })
}

module.exports = start

if (require.main === module) {
  const argv = yargs(process.argv.slice(2))
    .boolean("show-protocol-logs")
    .alias("show-protocol-logs", ["P"])
    .describe("show-protocol-logs", "Print the client protocol logs to stdout.")
    .help()
    .version().argv

  start({ SHOW_PROTOCOL_LOGS: argv.showProtocolLogs ? "true" : "false" })
}
