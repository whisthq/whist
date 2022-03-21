/*
  Build config
  This file exports the variables used at build time
*/

const { WhistEnvironments } = require("./constants")
const envOverrides = require("./envOverrides")
const assert = require("assert").strict

// Set the CORE_TS_ENVIRONMENT environment variable so that it can be parsed by core-ts. This
// is required so that the core-ts environment matches the client-applications environment in
// deploy/packaged scenarios.
if (!process.env.CORE_TS_ENVIRONMENT) {
  process.env.CORE_TS_ENVIRONMENT = envOverrides.appEnvironment
}

const { appEnvironment = WhistEnvironments.LOCAL } = envOverrides

// Make sure we have a valid appEnvironment
assert(
  appEnvironment === WhistEnvironments.LOCAL ||
    appEnvironment === WhistEnvironments.DEVELOPMENT ||
    appEnvironment === WhistEnvironments.STAGING ||
    appEnvironment === WhistEnvironments.PRODUCTION
)

// Icon name
const iconName =
  appEnvironment === WhistEnvironments.PRODUCTION ||
  appEnvironment === WhistEnvironments.LOCAL
    ? "icon"
    : `icon_${appEnvironment}`

module.exports = {
  appEnvironment,
  iconName,
}
