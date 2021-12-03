// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const start = (env) => {
  helpers.removeEnvOverridesFile()

  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()
  helpers.snowpackDev({
    ...env,
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
    .option("initial-url", {
      description: "The URL to open upon Whist browser startup",
      alias: ["U"],
      type: "string",
      requiresArg: true,
      demandOption: false,
      default: "",
    })
    .help().argvs
  start(
    {
      SHOW_PROTOCOL_LOGS: argv.showProtocolLogs,
      INITIAL_URL: argv.initialUrl,
      DEVELOPMENT_ENV: argv.useLocalServer ? "local" : "dev",
    }
  )
}
