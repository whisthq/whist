/*
  Deploys the configuration defined by tenant.yaml to a given Auth0 tenant.

  To be called via:
  yarn deploy:[env]
*/

const { execSync } = require("child_process")

// List of environment variables that must be defined in order for deploy to succeed.
const REQUIRED_ENV_VARS = ["GOOGLE_OAUTH_SECRET", "APPLE_OAUTH_SECRET"]

REQUIRED_ENV_VARS.forEach((v) => {
    if (!process.env[v]) {
        console.log(`Environment variable "${v}" not defined`)
        process.exit()
    }
})

const env = process.argv[2]

// Deploy to Auth0 tenant
execSync(
    `a0deploy import --input_file tenant.yaml --config_file ./config/${env}.json`,
    { stdio: "inherit" }
)
