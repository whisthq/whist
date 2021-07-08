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
  local: "https://fractal-project-aws-refactor.herokuapp.com",
  dev: "https://fractal-project-aws-refactor.herokuapp.com",
  staging: "https://staging-server.fractal.co",
  production: "https://prod-server.fractal.co",
}

module.exports = {
  FractalNodeEnvironments,
  FractalEnvironments,
  FractalWebservers,
}
