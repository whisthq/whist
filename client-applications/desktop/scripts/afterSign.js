require('dotenv').config()
const { notarize } = require('electron-notarize')
const fs = require('fs')

exports.default = async function afterSign(context) {
    const { electronPlatformName, appOutDir } = context
    if (electronPlatformName !== 'darwin') {
        return
	exports.default = function moveLoadingandRename(context) {
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