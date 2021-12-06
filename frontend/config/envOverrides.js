/*
  Config overrides
  This file exports the environment overrides provided by `env_overrides.json`.
*/

let envOverrides = {}

try {
  envOverrides = require("../env_overrides.json") || {}
} catch (err) {
  console.log(`No "env_overrides.json" file found.`)
}

module.exports = envOverrides
