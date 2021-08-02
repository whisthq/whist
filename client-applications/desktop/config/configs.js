/*
  Environment configs
  This file exports environment-specific variables used at build and runtime.

  This is the only file in this directory (except build.js) that should be
  imported by any file outside this directory; it re-exports anything that
  needs to be used by anything in `src/`, instance.
*/

const {
  FractalEnvironments,
  FractalNodeEnvironments,
  FractalWebservers,
} = require("./constants")

const {
  protocolName,
  protocolFolder,
  loggingFiles,
  userDataFolderNames,
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
    keys,
    protocolName,
    protocolFolder,
    buildRoot,
    url: {
      WEBSERVER_URL: FractalWebservers.dev,
      FRONTEND_URL: "https://dev.fractal.co",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "j1toifpKVu5WA6YbhEqBnMWN0fFxqw5I", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "dev",
    sentryEnv: "development",
    clientDownloadURLs: {
      MacOS: "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
      Windows:
        "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe",
    },
    title: "Fractal (local)",
  },
  DEVELOPMENT: {
    appEnvironment,
    keys,

    protocolName,
    protocolFolder,
    buildRoot,
    url: {
      WEBSERVER_URL: FractalWebservers.dev,
      FRONTEND_URL: "https://dev.fractal.co",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "j1toifpKVu5WA6YbhEqBnMWN0fFxqw5I", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "dev",
    sentryEnv: "development",
    clientDownloadURLs: {
      MacOS: "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
      Windows:
        "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe",
    },
    title: "Fractal (development)",
  },
  STAGING: {
    appEnvironment,
    keys,
    protocolName,
    protocolFolder,
    buildRoot,
    url: {
      WEBSERVER_URL: FractalWebservers.staging,
      FRONTEND_URL: "https://staging.fractal.co",
    },
    auth0: {
      auth0Domain: "fractal-staging.us.auth0.com",
      clientId: "WXO2cphPECuDc7DkDQeuQzYUtCR3ehjz", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "staging",
    sentryEnv: "staging",
    clientDownloadURLs: {
      MacOS:
        "https://fractal-chromium-macos-staging.s3.amazonaws.com/Fractal.dmg",
      Windows:
        "https://fractal-chromium-windows-staging.s3.amazonaws.com/Fractal.exe",
    },
    title: "Fractal (staging)",
  },
  PRODUCTION: {
    appEnvironment,
    keys,
    protocolName,
    protocolFolder,
    buildRoot,
    url: {
      WEBSERVER_URL: FractalWebservers.production,
      FRONTEND_URL: "https://fractal.co",
    },
    auth0: {
      auth0Domain: "fractal-prod.us.auth0.com",
      clientId: "Ulk5B2RfB7mM8BVjA3JtkrZT7HhWIBLD", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
    deployEnv: "prod",
    sentryEnv: "production",
    clientDownloadURLs: {
      MacOS: "https://fractal-chromium-macos-prod.s3.amazonaws.com/Fractal.dmg",
      Windows:
        "https://fractal-chromium-windows-base.s3.amazonaws.com/Fractal.exe",
    },
    title: "Fractal",
  },
}

module.exports = {
  appEnvironment,
  iconName,
  configs,
  loggingFiles,
  userDataFolderNames,
  FractalNodeEnvironments,
  FractalEnvironments,
  FractalWebservers,
}
