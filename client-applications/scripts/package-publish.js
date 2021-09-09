// Package and publish the app to S3. Note that this implies notarization (on
// supported platforms).

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const packageNotarize = (env, config, version, environment, commit) => {
  // If we're passed a --config CLI argument, we'll use that as the JSON
  // config value. If no --config argument, we'll build the config ourselves.
  if (!config) config = helpers.buildConfig({ deploy: environment })

  helpers.reinitializeYarn()
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()

  helpers.configureCodeSigning(true)

  helpers.setPackagedEnv(environment)

  // Add the config to env_overrides.json
  helpers.setPackagedConfig(config)

  // We hardcode the commit sha to the current commit
  helpers.setPackagedCommitSha(commit)

  helpers.populateSecretKeys([
    "AWS_ACCESS_KEY",
    "AWS_SECRET_KEY",
    "AMPLITUDE_KEY",
  ])

  helpers.snowpackBuild({
    ...env,
    CONFIG: config,
    VERSION: version,
    COMMIT_SHA: commit,
  })

  const getBucketName = () => {
    let osStr
    switch (process.platform) {
      case "darwin":
        osStr = "macos"
        break
      case "win32":
        osStr = "windows"
        break
      case "linux":
        osStr = "ubuntu"
        break
    }
    return `fractal-chromium-${osStr}-${environment}`
  }

  helpers.electronPublish(getBucketName())

  helpers.removeEnvOverridesFile()
}

module.exports = packageNotarize

if (require.main === module) {
  // Get required args
  const argv = yargs(process.argv.slice(2))
    .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
    .option("config", {
      description: "The JSON object output from fractal/config",
      type: "string",
    })
    .option("version", {
      description:
        "Set the version number of the client app to be published. Must be greater than the current version in the S3 bucket for autoupdate to work.",
      type: "string",
      requiresArg: true,
      demandOption: true,
    })
    .option("environment", {
      description: "Set the client-app environment (dev, staging, or prod).",
      type: "string",
      requiresArg: true,
      choices: ["dev", "staging", "prod"],
      demandOption: true,
    })
    .option("commit", {
      description: "Set the commit hash of the current branch",
      type: "string",
      requiresArg: true,
      demandOption: true,
    })
    .help().argv

  packageNotarize({}, argv.config, argv.version, argv.environment, argv.commit)
}
