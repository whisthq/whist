// Package the app for local testing using snowpack and electron-builder

const helpers = require("./build-package-helpers")

const packageLocal = (env) => {
  // Retrieve the config variables for the associated environment
  const config = helpers.getConfig({ deploy: "dev" })

  helpers.buildAndCopyProtocol(true)
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
  packageLocal({})
}
