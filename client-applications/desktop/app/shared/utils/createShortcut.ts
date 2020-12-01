export default async (app: any) => {
    const os = require("os")
    const platform = os.platform()
    const app_url = `fractal://${app.app_id.toLowerCase.replace(/\s+/g, "-")}`

    if (platform === "darwin") {
        console.log("mac shortcuts not yet implemented")
    } else if (platform === "linux") {
        console.log("linux shortcuts not yet implemented")
    } else if (platform === "win32") {
        const ws = require("windows-shortcuts")
        ws.create(`%UserProfile%\Desktop\${app.app_id} on Fractal.lnk`, {
            target: app_url,
            desc: app.description,
            // icon, iconIndex
        })
    } else {
        console.log(`no suitable os found, instead got ${platform}`)
    }
}
