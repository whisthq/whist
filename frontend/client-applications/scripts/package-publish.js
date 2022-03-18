// Package and publish the app to S3. Note that this implies notarization (on
// supported platforms).

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const packageNotarize = (env, version, environment, commit) => {
  // Retrieve the config variables for the associated environment
  const config = helpers.getConfig({ deploy: environment })

  helpers.reinitializeYarn()
  helpers.buildAndCopyProtocol(true)
  helpers.buildTailwind()

  helpers.configureCodeSigning(true)

  helpers.setPackagedEnv(environment)

  // Add the config to env_overrides.json
  helpers.setPackagedConfig(config)

  // We hardcode the commit sha to the current commit
  helpers.setPackagedCommitSha(commit)

  helpers.populateSecretKeys(["AMPLITUDE_KEY"])

  helpers.snowpackBuild({
    ...env,
    CONFIG: config,
    VERSION: version,
    COMMIT_SHA: commit,
  })

  helpers.electronPublish(environment)

  helpers.removeEnvOverridesFile()
}

module.exports = packageNotarize

if (require.main === module) {
  // Get required args
  const argv = yargs(process.argv.slice(2))
    .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
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
      description: "Set the commit hash of the current branch.",
      type: "string",
      requiresArg: true,
      demandOption: true,
    })
    .help().argv

  packageNotarize({}, argv.version, argv.environment, argv.commit)
}
