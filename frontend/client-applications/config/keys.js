/*
  Config access keys
  This file exports any access keys used by the client app code, populated by `config/overrides`.

  These access keys are populated in CI from GitHub Secrets. None of these
  should be necessary for local development. Also note that notarization and
  publishing from developer machines is not possible by design.
*/

const envOverrides = require("./envOverrides")

const {
  AMPLITUDE_KEY = "PLACEHOLDER_CLIENTAPP_AMPLITUDE_KEY",
  COMMIT_SHA = "PLACEHOLDER_CLIENTAPP_COMMIT_SHA",
} = envOverrides

const keys = {
  AMPLITUDE_KEY,
  COMMIT_SHA,
}

module.exports = {
  keys,
}
