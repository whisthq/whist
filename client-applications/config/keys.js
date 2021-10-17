/*
  Config access keys
  This file exports any access keys used by the client app code, populated by `config/overrides`.

  These access keys are populated in CI from GitHub Secrets. None of these
  should be necessary for local development. Also note that notarization and
  publishing from developer machines is not possible by design.
*/

const envOverrides = require("./envOverrides")

const {
  AWS_ACCESS_KEY = "PLACEHOLDER_CLIENTAPP_AWS_ACCESS_KEY", // only permissioned for S3 client-apps buckets
  AWS_SECRET_KEY = "PLACEHOLDER_CLIENTAPP_AWS_SECRET_KEY",
  AMPLITUDE_KEY = "PLACEHOLDER_CLIENTAPP_AMPLITUDE_KEY",
  COMMIT_SHA = "PLACEHOLDER_CLIENTAPP_COMMIT_SHA",
} = envOverrides

const keys = {
  AWS_ACCESS_KEY,
  AWS_SECRET_KEY,
  AMPLITUDE_KEY,
  COMMIT_SHA,
}

module.exports = {
  keys,
}
