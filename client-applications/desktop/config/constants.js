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
    local: "http://127.0.0.1:7730",
    dev: "https://dev-server.fractal.co",
    staging: "https://staging-server.fractal.co",
    production: "https://prod-server.fractal.co",
}

module.exports = {
    FractalNodeEnvironments,
    FractalEnvironments,
    FractalWebservers,
}
