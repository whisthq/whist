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

    // codesign the two external binaries defined above, Fractal and FractalLauncher (all binaries need to be codesigned for notarizing to work)
    exec(
        'codesign -f -v -s \'Fractal Computers, Inc.\' "${appOutDir}/${appName}.app/Contents/MacOS/FractalLauncher"',
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
    exec(
        'codesign -f -v -s \'Fractal Computers, Inc.\' "${appOutDir}/${appName}.app/Contents/MacOS/Fractal"',
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
