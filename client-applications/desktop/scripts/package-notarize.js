// Package the app for testing, but also notarize it. Note that we require a
// version to test for notarization testing so that we can also test the
// version-handling logic.

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const packageNotarize = (env, version, commit, ..._args) => {
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

  helpers.snowpackBuild({ ...env, VERSION: version, COMMIT_SHA: commit })
  helpers.electronBuild()

  helpers.removeEnvOverridesFile()
}

module.exports = packageNotarize

if (require.main === module) {
  // We require the version argument for notarization-level testing so that at
  // least some of our argument handling is covered by CI as well.
  const argv = yargs(process.argv.slice(2))
    .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
    .option("version", {
      description:
        "Set the version number of the client app for notarization testing.",
      type: "string",
      requiresArg: true,
      demandOption: true,
    })
    .option("commit", {
      description: "Set the commit hash of the current branch",
      type: "string",
      requiresArg: true,
      demandOption: true,
    })
    .help().argv

  packageNotarize({}, argv.version, argv.commit)
}
