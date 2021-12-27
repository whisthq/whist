// Build the protocol, build tailwind, and run `snowpack dev`

const helpers = require("./build-package-helpers");
const yargs = require("yargs");

const start = (env, config) => {
  helpers.removeEnvOverridesFile();

  helpers.snowpackDev({
    ...env,
    CONFIG: config,
  });
};

module.exports = start;

if (require.main === module) {
  const argv = yargs(process.argv.slice(2))
    .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
    .option("show-protocol-logs", {
      description: "Print the client protocol logs to stdout",
      alias: ["P"],
      type: "boolean",
    })
    .help().argv;
  start(
    { SHOW_PROTOCOL_LOGS: argv.showProtocolLogs ? "true" : "false" },
    argv.config
  );
}
