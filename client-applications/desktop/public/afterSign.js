require('dotenv').config()
const { notarize } = require('electron-notarize')
const fs = require('fs')
const { exec } = require('child_process')

exports.default = async function afterSign(context) {
    const { electronPlatformName, appOutDir } = context
    if (electronPlatformName !== 'darwin') {
        return
    }

    const appName = context.packager.appInfo.productFilename

    // move loading files to correct location
    // (PNGs must be moved afterwards because otherwise the electron codesign freaks out)
    fs.renameSync(
        `${__dirname}/../loadingtemp`,
        `${appOutDir}/${appName}.app/Contents/MacOS/loading`
    )

    // rename client app to FractalLauncher and protocol to Fractal
    fs.renameSync(
        `${appOutDir}/${appName}.app/Contents/MacOS/Fractal`,
        `${appOutDir}/${appName}.app/Contents/MacOS/FractalLauncher`
    )
    fs.renameSync(
        `${appOutDir}/${appName}.app/Contents/MacOS/FractalClient`,
        `${appOutDir}/${appName}.app/Contents/MacOS/Fractal`
    )

    return await notarize({
        appBundleId: 'com.fractalcomputers.fractal',
        appPath: `${appOutDir}/${appName}.app`,
        appleId: `phil@fractal.co`,
        appleIdPassword: `seoy-fnou-zjro-xicr`,
    })
}