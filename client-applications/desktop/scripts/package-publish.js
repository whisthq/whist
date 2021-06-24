// Package and publish the app to S3. Note that this implies notarization (on
// supported platforms).

const helpers = require("./build-package-helpers")
const yargs = require("yargs")

const packageNotarize = (env, version, environment, commit) => {
  helpers.reinitializeYarn()
  helpers.buildAndCopyProtocol()
  helpers.buildTailwind()

  helpers.configureCodeSigning(true)

  helpers.setPackagedEnv(environment)
  helpers.populateSecretKeys([
    "AWS_ACCESS_KEY",
    "AWS_SECRET_KEY",
    "AMPLITUDE_KEY",
  ])

  helpers.snowpackBuild({ ...env, VERSION: version, COMMIT_SHA: commit})

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
      description: "Set the commit hash of the current branch",
      type: "string",
      requiresArg: true,
      demandOption: true,
    })
    .help().argv

  packageNotarize({}, argv.version, argv.environment, argv.commit)
}
