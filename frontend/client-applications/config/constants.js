/*
  Config constants
  This file contains the constants used for configuring the client application and build and runtime.
*/

const WhistEnvironments = {
  LOCAL: "local",
  DEVELOPMENT: "dev",
  STAGING: "staging",
  PRODUCTION: "prod",
}

const WhistNodeEnvironments = {
  DEVELOPMENT: "development",
  PRODUCTION: "production",
}

// Webserver URLs
const WhistWebservers = {
  local: "http://0.0.0.0:8082",
  dev: "https://dev-server.whist.com",
  staging: "https://staging-server.whist.com",
  production: "https://prod-server.whist.com",
}

module.exports = {
  WhistNodeEnvironments,
  WhistEnvironments,
  WhistWebservers,
}
