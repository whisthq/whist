// Package the app for local testing using snowpack and electron-builder

const yargs = require("yargs")
const helpers = require("./build-package-helpers")

const packageLocal = (env, config) => {
  // If we're passed a --config CLI argument, we'll use that as the JSON
  // config value. If no --config argument, we'll build the config ourselves.
  if (!config) {
    helpers.buildConfigContainer()
    config = helpers.getConfig({ deploy: "dev" })
  }
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()
  helpers.configureCodeSigning(false)

  // For testing, we just hardcode the environment to dev
  helpers.setPackagedEnv("local")

  // Add the config to env_overrides.json
  helpers.setPackagedConfig(config)

  // For package-local, we don't want to increment the version so we use existing version
  helpers.snowpackBuild({
    ...env,
    CONFIG: config,
    VERSION: helpers.getCurrentClientAppVersion(),
  })
  helpers.electronBuild()

  helpers.removeEnvOverridesFile()
}

module.exports = packageLocal

if (require.main === module) {
  // We require the version argument for notarization-level testing so that at
  // least some of our argument handling is covered by CI as well.
  const argv = yargs(process.argv.slice(2))
    .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
    .option("config", {
      description: "The JSON object output from fractal/config",
      type: "string",
    })
    .help().argv
  packageLocal({}, argv.config)
}
