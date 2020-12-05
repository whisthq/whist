import { OperatingSystem, FractalWindowsDirectory } from "shared/types/client"
import { FractalApp } from "shared/types/ui"
import { FractalNodeEnvironment } from "shared/types/config"
import { debugLog } from "shared/utils/logging"

export class SVGConverter {
    /*
        Description:
            Converts an SVG to various image formats. Currently supports
            converting to Base64 PNG and .ico.

        Usage: 
            const svgInput = "https://my-svg-url.svg"
            new SVGConverter().convertToPngBase64(svgInput, (pngBase64) => {
                console.log(pngBase64)
            })

        Methods:
            convertToPngBase64(input: string, callback: function) : Convert an svg (requires .svg) to base64 
            convertToIco(input: string, callback: function)L Convert an svg (requires .svg) to .ico
    */
    constructor() {
        this._init = this._init.bind(this)
        this._cleanUp = this._cleanUp.bind(this)
        this.convertToPngBase64 = this.convertToPngBase64.bind(this)
    }

    _init() {
        this.canvas = document.createElement("canvas")
        this.imgPreview = document.createElement("img")
        this.imgPreview.style =
            "position: absolute; top: -9999px; width: 64px; height: 64px;"

        document.body.appendChild(this.imgPreview)
        this.canvasCtx = this.canvas.getContext("2d")
    }

    _cleanUp() {
        document.body.removeChild(this.imgPreview)
    }

    convertToPngBase64(input: string, callback: (imgData: string) => void) {
        this._init()
        let _this = this
        this.imgPreview.onload = function () {
            const img = new Image()
            _this.canvas.width = _this.imgPreview.clientWidth
            _this.canvas.height = _this.imgPreview.clientHeight
            img.crossOrigin = "anonymous"
            img.src = _this.imgPreview.src
            img.onload = function () {
                _this.canvasCtx.drawImage(img, 0, 0)

                let imgData = _this.canvas.toDataURL("image/png")
                callback(imgData)
                _this._cleanUp()
            }
        }

        this.imgPreview.src = input
    }
}

export const createShortcutName = (appName: string): string => {
    /*
        Description:
            Generates a shortcut name for consistency

        Arguments:
            appName (string): Name of original app, i.e. "Figma" or "Google Chrome"
        
        Returns:
            name (string): Name of shortcut
    */
    return `Fractalized ${appName}`
}

export const createShortcut = (
    app: FractalApp,
    outputPath?: string
): boolean => {
    /*
        Description:
            Creates a shortcut to the Fractal streamed app

        Arguments:
            app (FractalApp): App to create shortcut to
            outputPath (string): Folder that the shortcut should be placed in. If not specified, defaults to Desktop.
        
        Returns:
            success (boolean): True/False if shortcut was created successfully
    */

    const tempPath = outputPath

    const os = require("os")
    const createDesktopShortcut = require("create-desktop-shortcuts")

    const platform: OperatingSystem = os.platform()
    const appURL = `fractal://${app.app_id.toLowerCase().replace(/\s+/g, "-")}`

    if (platform === OperatingSystem.MAC) {
        debugLog("Mac shortcuts not yet implemented")
        return false
    }
    if (platform === OperatingSystem.WINDOWS) {
        const vbsPath = `${require("electron")
            .remote.app.getAppPath()
            .replace(
                "app.asar",
                "app.asar.unpacked"
            )}\\node_modules\\create-desktop-shortcuts\\src\\windows.vbs`

        new SVGConverter().convertToPngBase64(app.logo_url, function (
            pngOutput: string
        ) {
            const toIco = require("to-ico")
            const fs = require("fs")
            const base64Data = pngOutput.replace("data:image/png;base64,", "")

            const binaryData = new Buffer(base64Data, "base64")

            toIco([binaryData]).then((buf) => {
                let path = require("electron").remote.app.getAppPath() + "\\"
                path = path.replace("\\resources\\app.asar", "")
                path = path.replace("\\app\\", "\\")

                createDirectorySync(path, "icons")
                const icoPath = `${path}\\icons\\${app.app_id}.ico`
                fs.writeFileSync(icoPath, buf)
                const shortcutCreated = createDesktopShortcut({
                    windows: {
                        outputPath: tempPath,
                        filePath: appURL,
                        name: createShortcutName(app.app_id),
                        vbsPath:
                            process.env.NODE_ENV ===
                            FractalNodeEnvironment.DEVELOPMENT
                                ? null
                                : vbsPath,
                        icon: icoPath,
                    },
                })
                console.log("DONE CREATING")
                return shortcutCreated
            })
        })

        console.log("LOG HERE")
        return true
    }
    debugLog(`no suitable os found, instead got ${platform}`)
    return false
}

export const checkIfShortcutExists = (shortcut: string): boolean => {
    /*
        Description:
            Checks to see if the user has the shortcut installed in their Desktop or Programs/Application folder.

        Arguments:
            shortcut (string): Name of shortcut (generated by createShortcutName)
        
        Returns:
            exists (boolean): True/False if the shortcut exists
    */

    const fs = require("fs")
    const os = require("os")

    const platform = os.platform()
    try {
        if (platform === OperatingSystem.MAC) {
            debugLog("mac shortcuts not yet implemented")
            return false
        }
        if (platform === OperatingSystem.WINDOWS) {
            const windowsDesktopPath = `${FractalWindowsDirectory.DESKTOP}${shortcut}.lnk`
            const windowsStartMenuPath = `${FractalWindowsDirectory.START_MENU}Fractal\\${shortcut}.lnk`

            const exists =
                fs.existsSync(windowsDesktopPath) ||
                fs.existsSync(windowsStartMenuPath)
            return exists
        }
        debugLog(`no suitable os found, instead got ${platform}`)
        return false
    } catch (err) {
        debugLog(err)
        return false
    }
}

export const createDirectorySync = (
    filePath: string,
    directoryName: string
): boolean => {
    /*
        Description:
            Creates a directory on the user's computer. 
        
        Usage:
            To create a directory called "Folder" in "C:\\User\\Fake_User", call createDirectorySync("C:\\User\\Fake_User", "Folder")

        Arguments:
            filePath (string): Folder that the new directory should be placed in
            directoryName (string): Name of new directory
        
        Returns:
            success: True/False if the directory was created successfully
    */

    const fs = require("fs")

    // If the base file path does not exist, return false
    if (!fs.existsSync(filePath) && fs.lstatSync(filePath).isDirectory()) {
        return false
    }

    // If the directory already exists, return true
    if (
        fs.existsSync(`${filePath}${directoryName}`) &&
        fs.lstatSync(`${filePath}${directoryName}`).isDirectory()
    ) {
        return true
    }

    // If the directory doesn't exist yet, create it
    fs.mkdirSync(`${filePath}${directoryName}`)
    return true
}
