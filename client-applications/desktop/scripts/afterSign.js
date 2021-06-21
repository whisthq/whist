// This file is referenced directly in the bundleConfig object of
// electron-builder.config.js. Electron-builder needs to pass the afterSign
// function a "context" object, and we don't have control of how it's called.
// For this reason, it doesn't conform to the (env, ...args) => {} signature
// that most other scripts have.
require("dotenv").config()
const { notarize } = require("electron-notarize")
const path = require("path")

exports.default = async function afterSign(context) {
  const { electronPlatformName, appOutDir } = context

  if (process.env.CSC_IDENTITY_AUTO_DISCOVERY === "false") return

  if (electronPlatformName === "darwin") {
    // These environment variables our set in our CI using GitHub Secrets.
    // Notarizing will automatically be tested in the client apps CI; you
    // should not need to notarize locally.
    // If you believe you do need to notarize locally, please check Notion
    // or ask for our Apple credentials.
    const appleApiKey = process.env.APPLE_API_KEY_ID ?? ""
    const appleApiKeyIssuer = process.env.APPLE_API_KEY_ISSUER_ID ?? ""
    const appName = context.packager.appInfo.productFilename

    if (appleApiKey && appleApiKeyIssuer) {
      return await notarize({
        appBundleId: "com.fractalcomputers.fractal",
        appPath: path.join(appOutDir, `${appName}.app`),
        appleApiKey: appleApiKey,
        appleApiIssuer: appleApiKeyIssuer,
      })
    } else {
      console.error(
        "Error: Attempted to notarize, but Apple API key environment not set."
      )
      process.exit(-1)
    }
  }
}
