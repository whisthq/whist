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
  WhistWebservers,
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
      WEBSERVER_URL: WhistWebservers.local,
      FRONTEND_URL: "https://dev.whist.com",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "F4M4J6VjbXlT2UzCfnxFaJK2yKJXbxcF", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "dev",
    clientDownloadURLs: {
      MacOS: "https://fractal-chromium-macos-dev.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://fractal-chromium-windows-dev.s3.amazonaws.com/Whist.exe",
    },
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
      WEBSERVER_URL: WhistWebservers.dev,
      FRONTEND_URL: "https://dev.whist.com",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "F4M4J6VjbXlT2UzCfnxFaJK2yKJXbxcF", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "dev",
    clientDownloadURLs: {
      MacOS: "https://fractal-chromium-macos-dev.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://fractal-chromium-windows-dev.s3.amazonaws.com/Whist.exe",
    },
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
      WEBSERVER_URL: WhistWebservers.staging,
      FRONTEND_URL: "https://staging.whist.com",
    },
    auth0: {
      auth0Domain: "fractal-staging.us.auth0.com",
      clientId: "WXO2cphPECuDc7DkDQeuQzYUtCR3ehjz", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "staging",
    clientDownloadURLs: {
      MacOS:
        "https://fractal-chromium-macos-staging.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://fractal-chromium-windows-staging.s3.amazonaws.com/Whist.exe",
    },
    title: "Whist (staging)",
  },
  PRODUCTION: {
    appEnvironment,
    protocolName,
    protocolFolder,
    buildRoot,
    url: {
      WEBSERVER_URL: WhistWebservers.production,
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
    clientDownloadURLs: {
      MacOS: "https://fractal-chromium-macos-prod.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://fractal-chromium-windows-base.s3.amazonaws.com/Whist.exe",
    },
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
  WhistWebservers,
}
