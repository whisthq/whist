/*
  Build config
  This file exports the variables used at build time
*/

const { FractalEnvironments } = require("./constants")
const envOverrides = require("./envOverrides")
const assert = require("assert").strict

// Set the CONFIG environment variable so that it can be parsed by core-ts.
process.env.CONFIG = envOverrides.CONFIG

const { appEnvironment = FractalEnvironments.LOCAL } = envOverrides

// Make sure we have a valid appEnvironment
assert(
    appEnvironment === FractalEnvironments.LOCAL ||
        appEnvironment === FractalEnvironments.DEVELOPMENT ||
        appEnvironment === FractalEnvironments.STAGING ||
        appEnvironment === FractalEnvironments.PRODUCTION
)

// Icon name
const iconName =
    appEnvironment === FractalEnvironments.PRODUCTION ||
    appEnvironment === FractalEnvironments.LOCAL
        ? "icon"
        : `icon_${appEnvironment}`

module.exports = {
    appEnvironment,
    iconName,
}
