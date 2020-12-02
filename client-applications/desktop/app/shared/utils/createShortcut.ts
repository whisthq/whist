export default (app: any) => {
    const os = require("os")
    const platform = os.platform()
    const app_url = `fractal://${app.app_id.toLowerCase().replace(/\s+/g, "-")}`

    if (platform === "darwin") {
        console.log("mac shortcuts not yet implemented")
    } else if (platform === "linux") {
        console.log("linux shortcuts not yet implemented")
    } else if (platform === "win32") {
        // path pointing to the vbs script in the packaged electron app
        const vbsPath =
            require("electron")
                .remote.app.getAppPath()
                .replace("app.asar", "app.asar.unpacked") +
            "\\node_modules\\@fractal\\create-desktop-shortcuts\\src\\windows.vbs"

        const createDesktopShortcut = require("create-desktop-shortcuts")

        const shortcutsCreated = createDesktopShortcut({
            windows: {
                filePath: app_url,
                name: app.app_id + " on Fractal",
                vbsPath: process.env.NODE_ENV === "development" ? "" : path,
            },
        })
    } else {
        console.log(`no suitable os found, instead got ${platform}`)
    }
}
