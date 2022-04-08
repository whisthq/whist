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
const WhistScalingServices = {
  local: "http://0.0.0.0:7730",
  dev: "https://dev-scaling-service.whist.com",
  staging: "https://staging-scaling-service.whist.com",
  production: "https://prod-scaling-service.whist.com",
}

module.exports = {
  WhistNodeEnvironments,
  WhistEnvironments,
  WhistScalingServices,
}
