const { app } = require("electron")
const {
  baseFilePath,
  protocolName,
  protocolFolder,
  loggingBaseFilePath,
  userDataFolderNames,
  buildRoot,
} = require("./paths")
const path = require("path")

const FractalNodeEnvironment = {
  DEVELOPMENT: "development",
  PRODUCTION: "production",
}

const FractalCIEnvironment = {
  LOCAL: "local",
  DEVELOPMENT: "dev",
  STAGING: "staging",
  PRODUCTION: "prod",
}

/*
    Webserver URLs
*/
const webservers = {
  local: "http://127.0.0.1:7730",
  dev: "http://dev-server.fractal.co",
  staging: "https://staging-server.fractal.co",
  production: "https://prod-server.fractal.co",
}

const keys = {
  AWS_ACCESS_KEY: "AKIA24A776SSHLVMSAVU", // only permissioned for S3 client-apps buckets
  AWS_SECRET_KEY: "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",
  LOGZ_API_KEY: "IroqVsvNytmNricZSTLUSVtJbxNYBgxp",
}

const stripeTest = {
  STRIPE_PUBLISH_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
  STRIPE_SECRET_KEY: "sk_test_6ndCgv5edtzMuyqMoBbt1gXj00xy90yd4L",
}

const stripeProd = {
  STRIPE_PUBLISH_KEY: "pk_live_XLjiiZB93KN0EjY8hwCxvKmB00whKEIj3U",
  STRIPE_SECRET_KEY: "sk_live_PYGk9BRcI3lXguj88Rj7IkN700WnaWd4sy",
}

/*
    All environment variables. All secret keys have permission restrictions.
*/
const environment = {
  LOCAL: {
    keys,
    baseFilePath,
    protocolName,
    protocolFolder,
    buildRoot,
    stripe: {
      ...stripeTest,
    },
    url: {
      WEBSERVER_URL: webservers.local,
      FRONTEND_URL: "http://localhost:3000",
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
    keys,
    baseFilePath,
    protocolName,
    protocolFolder,
    buildRoot,
    stripe: {
      ...stripeTest,
    },
    url: {
      WEBSERVER_URL: webservers.local,
      FRONTEND_URL: "https://dev.fractal.co",
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
    keys,
    baseFilePath,
    protocolName,
    protocolFolder,
    buildRoot,
    stripe: {
      ...stripeTest,
    },
    url: {
      WEBSERVER_URL: webservers.staging,
      FRONTEND_URL: "https://staging.fractal.co",
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
    keys,
    baseFilePath,
    protocolName,
    protocolFolder,
    buildRoot,
    stripe: {
      ...stripeProd,
    },
    url: {
      WEBSERVER_URL: webservers.production,
      FRONTEND_URL: "https://fractal.co",
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
  FractalNodeEnvironment,
  FractalCIEnvironment,
  webservers,
  environment,
  loggingBaseFilePath,
  userDataFolderNames,
}
