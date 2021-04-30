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
    const appleId = process.env.FRACTAL_NOTARIZE_APPLE_ID ?? ""
    const applePassword = process.env.FRACTAL_NOTARIZE_APPLE_PASSWORD ?? ""

    const appName = context.packager.appInfo.productFilename

    if (appleId && applePassword) {
      return await notarize({
        appBundleId: "com.fractalcomputers.fractal",
        appPath: path.join(appOutDir, `${appName}.app`),
        appleId: appleId,
        appleIdPassword: applePassword,
      })
    }
  }
}
