// Package the app for testing, but also notarize it. Note that we require a
// version to test for notarization testing so that we can also test the
// version-handling logic.

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const packageNotarize = (env, config, version) => {
  // If we're passed a --config CLI argument, we'll use that as the JSON
  // config value. If no --config argument, we'll build the config ourselves.
  if (!config) config = helpers.buildConfig({ deploy: "dev" })

  helpers.reinitializeYarn()
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()

  helpers.configureCodeSigning(true)

  // For testing, we just hardcode the environment to dev
  helpers.setPackagedEnv("dev")

  // We test setting the secret keys
  helpers.populateSecretKeys([
    "AWS_ACCESS_KEY",
    "AWS_SECRET_KEY",
    "AMPLITUDE_KEY",
  ])

  helpers.snowpackBuild({ ...env, CONFIG: config, VERSION: version })
  helpers.electronBuild()
}

module.exports = packageNotarize

if (require.main === module) {
  // We require the version argument for notarization-level testing so that at
  // least some of our argument handling is covered by CI as well.
  const argv = yargs(process.argv.slice(2))
    .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
    .option("config", {
      description: "The JSON object output from fractal/config",
      type: "string",
    })
    .option("version", {
      description:
        "Set the version number of the client app for notarization testing.",
      type: "string",
      requiresArg: true,
      demandOption: true,
    })
    .help().argv

  packageNotarize({}, argv.config, argv.version)
}
