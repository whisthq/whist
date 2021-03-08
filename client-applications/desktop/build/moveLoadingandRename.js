require('dotenv').config()
const fs = require("fs")

exports.default = function moveLoading(context) {
    const { electronPlatformName, appOutDir } = context
    if (electronPlatformName !== 'darwin') {
        return
    }

    const appName = context.packager.appInfo.productFilename

    // move loading files to correct location
    fs.renameSync(`${__dirname}/../loadingtemp`, `${appOutDir}/${appName}.app/Contents/MacOS/loading`)

    // rename client app to FractalLauncher and protocol to Fractal
    fs.renameSync(`${appOutDir}/${appName}.app/Contents/MacOS/Fractal`, `${appOutDir}/${appName}.app/Contents/MacOS/FractalLauncher`)
    fs.renameSync(`${appOutDir}/${appName}.app/Contents/MacOS/FractalClient`, `${appOutDir}/${appName}.app/Contents/MacOS/Fractal`)
}
