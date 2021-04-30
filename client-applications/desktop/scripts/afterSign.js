require("dotenv").config()
const { notarize } = require("electron-notarize")
const path = require("path")

exports.default = async function afterSign(context) {
  const { electronPlatformName, appOutDir } = context

  if (process.env.CSC_IDENTITY_AUTO_DISCOVERY === "false") return

  if (electronPlatformName === "darwin") {
    const appleId = process.env.FRACTAL_NOTARIZE_APPLE_ID ?? ""
    const applePassword = process.env.FRACTAL_NOTARIZE_APPLE_PASSWORD ?? ""

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
