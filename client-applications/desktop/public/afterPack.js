require("dotenv").config()
const fs = require("fs")
const { execSync } = require("child_process")

exports.default = function afterPack(context) {
    const { electronPlatformName, appOutDir } = context
    if (
        electronPlatformName !== "darwin" ||
        process.env.CSC_IDENTITY_AUTO_DISCOVERY === "false"
    ) {
        return
    }

    const appName = context.packager.appInfo.productFilename

    // execSync(`find ${appOutDir}/${appName}.app/Contents/protocol-build/client -exec codesign -f -v -s "Fractal Computers, Inc." {} \\;`,
    //     (error, stderr, stdout) => {
    //         if(error) console.log("Error", error)
    //         if(stderr) console.log("Stderr", stderr)
    //         console.log("Stdout", stdout)
    //     }
    // )

    execSync(
        `mv ${appOutDir}/${appName}.app/Contents/protocol-build/client ${appOutDir}/${appName}.app/Contents/MacOS`,
        (error, stderr, stdout) => {
            if (error) console.log("Error", error)
            if (stderr) console.log("Stderr", stderr)
            console.log("Stdout", stdout)
        }
    )
}
