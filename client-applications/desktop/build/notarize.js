require("dotenv").config()
const { notarize } = require("electron-notarize")
import { moveLoadingandRename } from "./moveLoadingandRename"

exports.default = async function notarizing(context) {
    const { electronPlatformName, appOutDir } = context
    if (electronPlatformName !== "darwin") {
        return
    }

    const appName = context.packager.appInfo.productFilename

    // move loading files to correct location
    moveLoadingandRename(context)

    return await notarize({
        appBundleId: "com.fractalcomputers.fractal",
        appPath: `${appOutDir}/${appName}.app`,
        appleId: `phil@fractal.co`,
        appleIdPassword: `seoy-fnou-zjro-xicr`,
    })
}
