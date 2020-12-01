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

        const createDesktopShortcut = require("create-desktop-shortcuts")

        const shortcutsCreated = createDesktopShortcut({
            windows: { filePath: "C:\\Windows\\system32\\notepad.exe" },
        })
    } else {
        console.log(`no suitable os found, instead got ${platform}`)
    }
}
