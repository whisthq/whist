// Package and publish the app to S3. Note that this implies notarization (on
// supported platforms).

const helpers = require("./build-package-helpers")
const yargs = require('yargs')

// Get required args
const argv = yargs(process.argv.slice(2))
  .version(false) // necessary to prevent mis-parsing of the `--version` arg we pass in
  .option('version', {
    description: "Set the version number of the client app to be published. Must be greater than the current version in the S3 bucket for autoupdate to work.",
    type: 'string',
    requiresArg: true,
    demandOption: true,
  })
  .option('environment', {
    description: "Set the client-app environment (dev, staging, or prod).",
    type: 'string',
    requiresArg: true,
    choices: ['dev', 'staging', 'prod'],
    demandOption: true,
  })
  .help()
  .argv


helpers.buildAndCopyProtocol()
helpers.configureCodeSigning(true)
helpers.reinitializeYarn()

helpers.setPackageEnv(argv.environment)

helpers.buildTailwind()

helpers.snowpackBuild(argv.version)

const getBucketName = () => {
  var os_str
  switch(process.platform) {
    case 'darwin':
      os_str = 'macos'
      break;
    case 'win32':
      os_str = 'windows'
      break;
    case 'linux': 
      os_str = 'ubuntu'
      break;
  }
  return 'fractal-chromium-' + os_str + '-' + argv.environment
}

helpers.electronPublish(getBucketName())
