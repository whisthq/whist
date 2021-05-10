/*
  Build config
  This file contains config variables used at build time
*/

const env = require("../env.json") || {}

// Environment-specific configuration

const { PACKAGED_ENV = "prod" } = env
const iconName = PACKAGED_ENV === "prod" ? "icon" : `icon_${PACKAGED_ENV}`

module.exports = {
  iconName,
  PACKAGED_ENV,
}
