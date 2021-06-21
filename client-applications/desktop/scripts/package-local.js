// Package the app for local testing using snowpack and electron-builder

export default function packageLocal(env, ..._args) {
  const helpers = require("./build-package-helpers")

  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()
  helpers.configureCodeSigning(false)

  // For testing, we just hardcode the environment to dev
  helpers.setPackagedEnv("dev")

  // For package-local, we don't want to increment the version so we use existing version
  helpers.snowpackBuild({ ...env, VERSION: helpers.getCurrentClientAppVersion() })
  helpers.electronBuild()
}

if (require.main === module) {
  packageLocal()
}
