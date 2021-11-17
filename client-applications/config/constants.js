/*
  Config constants
  This file contains the constants used for configuring the client application and build and runtime.
*/

const FractalEnvironments = {
  LOCAL: "local",
  DEVELOPMENT: "dev",
  STAGING: "staging",
  PRODUCTION: "prod",
}

const FractalNodeEnvironments = {
  DEVELOPMENT: "development",
  PRODUCTION: "production",
}

// Webserver URLs
const FractalWebservers = {
  local: "https://dev-server.whist.com",
  dev: "https://dev-server.whist.com",
  staging: "https://staging-server.whist.com",
  production: "https://prod-server.whist.com",
}

module.exports = {
  FractalNodeEnvironments,
  FractalEnvironments,
  FractalWebservers,
}
