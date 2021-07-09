// Package the app for local testing using snowpack and electron-builder

const helpers = require("./build-package-helpers")

const packageLocal = (env, ..._args) => {
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()
  helpers.configureCodeSigning(false)

  // For testing, we just hardcode the environment to dev
  helpers.setPackagedEnv("local")

  // For package-local, we don't want to increment the version so we use existing version
  helpers.snowpackBuild({
    ...env,
    VERSION: helpers.getCurrentClientAppVersion(),
  })
  helpers.electronBuild()

  helpers.removeEnvOverridesFile()
}

module.exports = packageLocal

if (require.main === module) {
  packageLocal()
}
