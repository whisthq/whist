// Run `snowpack dev` skipping the build steps for the protocol and tailwind.

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const start = (env, config) => {
  helpers.removeEnvOverridesFile()

  // // If we're passed a --config CLI argument, we'll use that as the JSON
  // // config value. If no --config argument, we'll parse the config ourselves.
  if (!config) config = helpers.getConfig({ deploy: "local" })

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
    .option("config", {
      description: "The JSON object output from fractal/config",
      type: "string",
    })
    .option("show-protocol-logs", {
      description: "Print the client protocol logs to stdout",
      alias: ["P"],
      type: "boolean",
    })
    .help().argv
  start(
    { SHOW_PROTOCOL_LOGS: argv.showProtocolLogs ? "true" : "false" },
    argv.config
  )
}
