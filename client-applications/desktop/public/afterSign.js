require('dotenv').config()
const { notarize } = require('electron-notarize')
const { exec } = require("child_process")

exports.default = async function afterSign(context) {
    const { electronPlatformName, appOutDir } = context
    if (electronPlatformName !== 'darwin' || process.env.CSC_IDENTITY_AUTO_DISCOVERY === 'false') {
        return
    }

    const appName = context.packager.appInfo.productFilename

    exec(
        "codesign -f -v -s 'Fractal Computers, Inc.' \"${appOutDir}/${appName}.app/Contents/MacOS/FractalLauncher\"",
        (error, stdout, stderr) => {
            if (error) {
                console.log(`error: ${error.message}`)
                return
            }
            if (stderr) {
                console.log(`stderr: ${stderr}`)
                return
            }
            console.log(`stdout: ${stdout}`)
        }
    )
    
    return await notarize({
        appBundleId: 'com.fractalcomputers.fractal',
        appPath: `${appOutDir}/${appName}.app`,
        appleId: `phil@fractal.co`,
        appleIdPassword: `seoy-fnou-zjro-xicr`,
    })
}