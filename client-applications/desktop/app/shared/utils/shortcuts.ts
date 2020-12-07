import { OperatingSystem, FractalWindowsDirectory } from "shared/types/client"
import { FractalApp } from "shared/types/ui"
import { FractalNodeEnvironment } from "shared/types/config"
import { debugLog } from "shared/utils/logging"

// Import with require() packages that require Node 10 experimental
const fs = require("fs")
const os = require("os")
const toIco = require("to-ico")

// How big should the PNG's be
const PNG_WIDTH = 64
const PNG_HEIGHT = 64

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

export class SVGConverter {
    /*
        Description:
            Converts an SVG to various image formats. Currently supports
            converting to Base64 PNG and .ico.

        Usage: 
            const svgInput = "https://my-svg-url.svg"
            const base64 = await SVGConverter.convertToPngBase64(svgInput)
            console.log(base64)

        Methods:
            base64PngToBuffer(input: string) :
                Converts a base64 encoded PNG to an ArrayBuffer and returns the ArrayBuffer
            convertToPngBase64(input: string) :
                Converts an svg (requires .svg) to base64 and returns a Promise with the base64 string
            convertToIco(input: string) : 
                Converts an svg (requires .svg) to .ico and returns a Promise with the ArrayBuffer
    */
    private static canvas: HTMLCanvasElement = document.createElement("canvas")

    private static imgPreview: HTMLImageElement = document.createElement("img")

    private static canvasCtx: CanvasRenderingContext2D | null = SVGConverter.canvas.getContext(
        "2d"
    )

    private static init() {
        document.body.appendChild(this.imgPreview)
    }

    private static cleanUp() {
        document.body.removeChild(this.imgPreview)
    }

    private static waitForImageToLoad(img: HTMLImageElement) {
        return new Promise((resolve, reject) => {
            img.onload = () => resolve(img)
            img.onerror = reject
        })
    }

    static base64PngToBuffer(base64: string) {
        base64 = base64.replace("data:image/png;base64,", "")
        const buffer = Buffer.from(base64, "base64")
        return buffer
    }

    static async convertToPngBase64(input: string): Promise<string> {
        // String where base64 output will be written to
        let base64 = ""

        // Load the SVG into HTML image element
        this.init()

        this.imgPreview.style.position = "absolute"
        this.imgPreview.style.top = "-9999px"
        this.imgPreview.src = input

        // Wait for SVG to load into HTML image preview
        await this.waitForImageToLoad(this.imgPreview)

        // Create final image
        const img = new Image()
        this.canvas.width = PNG_WIDTH
        this.canvas.height = PNG_HEIGHT
        img.crossOrigin = "anonymous"
        img.src = this.imgPreview.src

        // Draw SVG onto canvas with resampling (to resize to 64 x 64)
        await this.waitForImageToLoad(img)

        if (this.canvasCtx) {
            this.canvasCtx.drawImage(
                img,
                0,
                0,
                img.width,
                img.height,
                0,
                0,
                this.canvas.width,
                this.canvas.height
            )
            // Encode PNG as a base64 string
            base64 = this.canvas.toDataURL("image/png")
        }
        this.cleanUp()
        return base64
    }

    static async convertToIco(input: string): Promise<ArrayBuffer> {
        let buffer = new ArrayBuffer(0)
        const base64 = await this.convertToPngBase64(input)

        if (base64) {
            const convertedBuffer = this.base64PngToBuffer(base64)
            buffer = await toIco([convertedBuffer])
        }

        return buffer
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
    return `${appName} Fractalized`
}

export const createShortcut = async (
    app: FractalApp,
    outputPath: string
): Promise<boolean> => {
    /*
        Description:
            Creates a shortcut to the Fractal streamed app and stores the shortcut in a specified directory

        Arguments:
            app (FractalApp): App to create shortcut to
            outputPath (string): Folder that the shortcut should be placed in. If not specified, defaults to Desktop.
        
        Returns:
            success (boolean): True/False if shortcut was created successfully
    */

    const createDesktopShortcut = require("create-desktop-shortcuts")

    const platform: OperatingSystem = os.platform()
    // appURL is the protocol that the shortcut should run the open Fractal
    const appURL = `fractal://${app.app_id.toLowerCase().replace(/\s+/g, "-")}`

    if (platform === OperatingSystem.MAC) {
        debugLog("Mac shortcuts not yet implemented")
        return false
    }
    if (platform === OperatingSystem.WINDOWS) {
        // Points to the folder where windows.vbs is located (shortcut creation code)
        const vbsPath = `${require("electron")
            .remote.app.getAppPath()
            .replace(
                "app.asar",
                "app.asar.unpacked"
            )}\\node_modules\\create-desktop-shortcuts\\src\\windows.vbs`

        // Convert SVG into a .ico ArrayBuffer
        const buffer = await SVGConverter.convertToIco(app.logo_url)

        // Create directory called /icons to store the .ico if it doesn't already exist
        createDirectorySync(FractalWindowsDirectory.ROOT_DIRECTORY, "icons")
        const icoPath = `${FractalWindowsDirectory.ROOT_DIRECTORY}\\icons\\${app.app_id}.ico`
        // Write .ico into directory
        fs.writeFileSync(icoPath, buffer)
        // Save shortcut in outputPath
        const success = createDesktopShortcut({
            windows: {
                outputPath: outputPath,
                filePath: appURL,
                name: createShortcutName(app.app_id),
                vbsPath:
                    process.env.NODE_ENV === FractalNodeEnvironment.DEVELOPMENT
                        ? null
                        : vbsPath,
                icon: icoPath,
            },
        })
        // Fire callback with shortcut creation success True/False
        return success
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
    const platform = os.platform()
    try {
        if (platform === OperatingSystem.MAC) {
            debugLog("mac shortcuts not yet implemented")
            return false
        }
        if (platform === OperatingSystem.WINDOWS) {
            // Check the desktop folder and Start Menu Programs folder
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

export const createWindowsShortcuts = async (
    app: FractalApp,
    desktop = true,
    startMenu = true
): boolean => {
    const startMenuPath = `${FractalWindowsDirectory.START_MENU}Fractal\\`
    let desktopSuccess = true
    let startMenuSuccess = true

    // Create a Fractal directory in the Start Menu if one doesn't exist
    if (!createDirectorySync(FractalWindowsDirectory.START_MENU, "Fractal")) {
        return false
    }

    if (desktop) {
        desktopSuccess = await createShortcut(
            app,
            FractalWindowsDirectory.DESKTOP
        )
    }

    if (startMenu) {
        startMenuSuccess = await createShortcut(app, startMenuPath)
    }

    return desktopSuccess || startMenuSuccess
}
