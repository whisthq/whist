require('dotenv').config()
const { notarize } = require('electron-notarize')

exports.default = async function afterSign(context) {
    const { electronPlatformName, appOutDir } = context
    if (electronPlatformName !== 'darwin' || process.env.CSC_IDENTITY_AUTO_DISCOVERY === 'false') {
        return
    }

    const appName = context.packager.appInfo.productFilename

    return await notarize({
        appBundleId: 'com.fractalcomputers.fractal',
        appPath: `${appOutDir}/${appName}.app`,
        appleId: `phil@fractal.co`,
        appleIdPassword: `seoy-fnou-zjro-xicr`,
    })
}