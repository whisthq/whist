export default async (app: any) => {
    const os = require("os")
    const platform = os.platform()
    const app_url = `fractal://${app.app_id.toLowerCase().replace(/\s+/g, "-")}`

    if (platform === "darwin") {
        console.log("mac shortcuts not yet implemented")
    } else if (platform === "linux") {
        console.log("linux shortcuts not yet implemented")
    } else if (platform === "win32") {
        console.log("Windows detected")
        const sm = require("windows-shortcut-maker")

        // Creates an object to store any and all parameters to be passed to the Windows API
        const options = {
            filepath: "C:\\Windows\\system32\\notepad.exe",
        }

        // It creates a "GIMP" shortcut file in the desktop
        try {
            sm.makeSync(options)
        } catch (error) {
            console.log(error)
        }

        // ws.create(`C:\\Users\\ming_\\Desktop\\test.lnk`, {
        //     target: app_url,
        //     desc: app.description,
        //     // icon, iconIndex
        // })
        // console.log(`C:\\Users\\ming_\\Desktop\\test.lnk`)
    } else {
        console.log(`no suitable os found, instead got ${platform}`)
    }
}
