// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const start = (env) => {
  helpers.removeEnvOverridesFile()

  // Retrieve the config variables for the associated environment
  const config = helpers.getConfig({ deploy: "local" })

  helpers.buildAndCopyProtocol(true)
  helpers.buildTailwind()
  helpers.snowpackDev({
    ...env,
    CONFIG: config,
    VERSION: helpers.getCurrentClientAppVersion(),
  })
}

module.exports = start

if (require.main === module) {
  const argv = yargs(process.argv.slice(2))
    .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
    .option("show-protocol-logs", {
      description: "Print the client protocol logs to stdout",
      alias: ["P"],
      type: "boolean",
    })
    .option("use-local-server", {
      description: "If true, use 127.0.0.1:7730 instead of the dev server",
      type: "boolean",
    })
    .help().argv
  start({
    SHOW_PROTOCOL_LOGS: argv.showProtocolLogs,
    DEVELOPMENT_ENV: argv.useLocalServer ? "local" : "dev",
  })
}
