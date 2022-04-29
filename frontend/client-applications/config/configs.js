/*
  Environment configs
  This file exports environment-specific variables used at build and runtime.

  This is the only file in this directory (except build.js) that should be
  imported by any file outside this directory; it re-exports anything that
  needs to be used by anything in `src/`, instance.
*/

const {
  WhistEnvironments,
  WhistNodeEnvironments,
  WhistScalingServices,
} = require("./constants")

const {
  protocolName,
  protocolFolder,
  loggingFiles,
  buildRoot,
} = require("./paths")

const { keys } = require("./keys")
const { appEnvironment, iconName } = require("./build")

/*
    All environment variables.
*/
const configs = {
  LOCAL: {
    appEnvironment,
    protocolName,
    protocolFolder,
    buildRoot,
    keys: {
      ...keys,
      LOGZ_KEY: "MoaZIzGkBxpsbbquDpwGlOTasLqKvtGJ",
    },
    url: {
      SCALING_SERVICE_URL: WhistScalingServices.local,
      FRONTEND_URL: "https://dev.whist.com",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "F4M4J6VjbXlT2UzCfnxFaJK2yKJXbxcF", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "dev",
    title: "Whist (local)",
  },
  DEVELOPMENT: {
    appEnvironment,
    protocolName,
    protocolFolder,
    buildRoot,
    keys: {
      ...keys,
      LOGZ_KEY: "MoaZIzGkBxpsbbquDpwGlOTasLqKvtGJ",
    },
    url: {
      SCALING_SERVICE_URL: WhistScalingServices.dev,
      FRONTEND_URL: "https://dev.whist.com",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "F4M4J6VjbXlT2UzCfnxFaJK2yKJXbxcF", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "dev",
    title: "Whist (development)",
  },
  STAGING: {
    appEnvironment,
    protocolName,
    protocolFolder,
    buildRoot,
    keys: {
      ...keys,
      LOGZ_KEY: "WrlrWXFqDYqCNCXVwkmuOpQOvHNeqIiJ",
    },
    url: {
      SCALING_SERVICE_URL: WhistScalingServices.staging,
      FRONTEND_URL: "https://staging.whist.com",
    },
    auth0: {
      auth0Domain: "fractal-staging.us.auth0.com",
      clientId: "WXO2cphPECuDc7DkDQeuQzYUtCR3ehjz", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "staging",
    title: "Whist (staging)",
  },
  PRODUCTION: {
    appEnvironment,
    protocolName,
    protocolFolder,
    buildRoot,
    url: {
      SCALING_SERVICE_URL: WhistScalingServices.production,
      FRONTEND_URL: "https://whist.com",
    },
    keys: {
      ...keys,
      LOGZ_KEY: "dhwhpmrnfXZqNrilucOruibXgunbBqQJ",
    },
    auth0: {
      auth0Domain: "auth.whist.com",
      clientId: "Ulk5B2RfB7mM8BVjA3JtkrZT7HhWIBLD", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "prod",
    title: "Whist",
  },
}

module.exports = {
  appEnvironment,
  iconName,
  configs,
  loggingFiles,
  WhistNodeEnvironments,
  WhistEnvironments,
  WhistScalingServices,
}
