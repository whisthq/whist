/*
  Build config
  This file exports the variables used at build time
*/

const { WhistEnvironments } = require("./constants")
const envOverrides = require("./envOverrides")
const assert = require("assert").strict

// Set the CONFIG environment variable so that it can be parsed by core-ts.
if (!process.env.CONFIG) {
  process.env.CONFIG = envOverrides.CONFIG
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
