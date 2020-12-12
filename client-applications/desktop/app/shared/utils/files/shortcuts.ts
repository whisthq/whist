import { OperatingSystem, FractalDirectory } from "shared/types/client"
import { FractalApp } from "shared/types/ui"
import { FractalNodeEnvironment } from "shared/types/config"
import { SVGConverter } from "shared/utils/files/images"
import { debugLog } from "shared/utils/general/logging"

const fs = require("fs")
const path = require("path")
const os = require("os")
const dedent = require("dedent-js")

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
        fs.existsSync(path.join(filePath, directoryName)) &&
        fs.lstatSync(path.join(filePath, directoryName)).isDirectory()
    ) {
        return true
    }

    // If the directory doesn't exist yet, create it
    fs.mkdirSync(path.join(filePath, directoryName))
    return true
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

export const createWindowsShortcut = async (
    app: FractalApp,
    outputPath: string
): Promise<boolean> => {
    /*
        Description:
            Creates a shortcut to the Fractal streamed app and stores the shortcut in a specified directory, for Windows

        Arguments:
            app (FractalApp): App to create shortcut to
            outputPath (string): Folder that the shortcut should be placed in. If not specified, defaults to Desktop.
        
        Returns:
            success (boolean): True/False if shortcut was created successfully
    */

    const createDesktopShortcut = require("create-desktop-shortcuts")

    // appURL is the protocol that the shortcut should run the open Fractal
    const appURL = `fractal://${app.app_id.toLowerCase().replace(/\s+/g, "-")}`

    // Points to the folder where windows.vbs is located (shortcut creation code)
    const vbsPath = path.join(
        FractalDirectory.getRootDirectory(),
        "resources/app.asar.unpacked/node_modules",
        "create-desktop-shortcuts/src/windows.vbs"
    )

    // Convert SVG into a .ico ArrayBuffer
    const buffer = await SVGConverter.convertToIco(app.logo_url)

    // Create directory called /icons to store the .ico if it doesn't already exist
    createDirectorySync(FractalDirectory.getRootDirectory(), "icons")
    const icoPath = path.join(
        FractalDirectory.getRootDirectory(),
        `icons/${app.app_id}.ico`
    )
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

export const createMacShortcut = async (
    app: FractalApp,
    outputPath: string
): Promise<boolean> => {
    /*
        Description:
            Creates a dummy application that launches the Fractal streamed app and stores the shortcut in a specified directory, for Mac

        Arguments:
            app (FractalApp): App to create shortcut to
            outputPath (string): Folder that the shortcut should be placed in. If not specified, defaults to ~/Desktop.
        
        Returns:
            success (boolean): True/False if shortcut was created successfully
    */

    try {
        // appURL is the protocol that the shortcut should run the open Fractal
        const appURL = `fractal://${app.app_id
            .toLowerCase()
            .replace(/\s+/g, "-")}`
        const appFolderName = `${createShortcutName(app.app_id)}.app`

        // Create application folder
        createDirectorySync(outputPath, appFolderName)
        createDirectorySync(path.join(outputPath, appFolderName), "Contents")
        const appFolderContents = path.join(
            outputPath,
            appFolderName,
            "Contents"
        )

        createDirectorySync(appFolderContents, "Resources")

        const icnsBuffer = await SVGConverter.convertToIcns(app.logo_url)
        const icnsPath = path.join(appFolderContents, "Resources/shortcut.icns")
        fs.writeFileSync(icnsPath, icnsBuffer)

        createDirectorySync(appFolderContents, "MacOS")

        const runScriptData = dedent(`
            #!/bin/bash
            # autogenerated file - do not edit

            open ${appURL}
        `)

        const runScriptPath = path.join(appFolderContents, "MacOS/run.sh")
        fs.writeFileSync(runScriptPath, runScriptData)
        fs.chmodSync(runScriptPath, 0o755)

        // based on steam's shortcut implementation
        const infoPlistData = dedent(`
            <?xml version="1.0" encoding="UTF-8"?>
            \t<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
            \t<plist version="1.0">
            \t<dict>
            \t\t<key>CFBundleExecutable</key>
            \t\t<string>run.sh</string>
            \t\t<key>CFBundleIconFile</key>
            \t\t<string>shortcut.icns</string>
            \t\t<key>CFBundleInfoDictionaryVersion</key>
            \t\t<string>1.0</string>
            \t\t<key>CFBundlePackageType</key>
            \t\t<string>APPL</string>
            \t\t<key>CFBundleSignature</key>
            \t\t<string>????</string>
            \t\t<key>CFBundleVersion</key>
            \t\t<string>1.0</string>
            \t\t<key>CFBundleShortVersionString</key>
            \t\t<string>Fractal</string>    
            \t</dict>
            \t</plist>
        `)

        const infoPlistPath = path.join(appFolderContents, "Info.plist")
        fs.writeFileSync(infoPlistPath, infoPlistData)
    } catch (err) {
        debugLog(err)
        return false
    }
    return true
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
            const macDesktopPath = path.join(
                FractalDirectory.DESKTOP,
                `${shortcut}.app`
            )
            const macApplicationsPath = path.join(
                FractalDirectory.MAC_APPLICATIONS,
                `Fractal/${shortcut}.app`
            )

            const exists =
                fs.existsSync(macDesktopPath) ||
                fs.existsSync(macApplicationsPath)
            return exists
        }
        if (platform === OperatingSystem.WINDOWS) {
            // Check the desktop folder and Start Menu Programs folder
            const windowsDesktopPath = path.join(
                FractalDirectory.DESKTOP,
                `${shortcut}.lnk`
            )
            const windowsStartMenuPath = path.join(
                FractalDirectory.WINDOWS_START_MENU,
                `Fractal/${shortcut}.lnk`
            )

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

export const createShortcut = async (app: FractalApp): Promise<boolean> => {
    /*
        Description:
            Creates a shortcut to the Fractal streamed app (cross-platform)

        Arguments:
            app (FractalApp): App to create shortcut to
        
        Returns:
            success (boolean): True/False if shortcut was created successfully
    */
    const platform = os.platform()
    if (platform === OperatingSystem.MAC) {
        // On Mac, create a shortcut in the User Application folder
        if (
            !createDirectorySync(FractalDirectory.MAC_APPLICATIONS, "Fractal")
        ) {
            return false
        }

        const applicationsStatus = await createMacShortcut(
            app,
            path.join(FractalDirectory.MAC_APPLICATIONS, "Fractal")
        )

        return applicationsStatus
    }
    if (platform === OperatingSystem.WINDOWS) {
        // On Windows, create a desktop shortcut and Start Menu shortcut
        if (
            !createDirectorySync(FractalDirectory.WINDOWS_START_MENU, "Fractal")
        ) {
            return false
        }

        const desktopStatus = await createWindowsShortcut(
            app,
            FractalDirectory.DESKTOP
        )

        const startMenuStatus = await createWindowsShortcut(
            app,
            path.join(FractalDirectory.WINDOWS_START_MENU, "Fractal")
        )

        return desktopStatus || startMenuStatus
    }
    debugLog(`no suitable os found, instead got ${platform}`)
    return false
}

export const deleteShortcut = (app: FractalApp): boolean => {
    /*
        Description:
            Deletes an app's shortcuts (cross-platform)

        Arguments:
            app (FractalApp): App to create shortcut to
        
        Returns:
            success (boolean): True/False if shortcut was deleted successfully
    */

    const shortcut = createShortcutName(app.app_id)
    const platform = os.platform()

    if (platform === OperatingSystem.MAC) {
        const macDesktopPath = path.join(
            FractalDirectory.DESKTOP,
            `${shortcut}.app`
        )
        const macApplicationsPath = path.join(
            FractalDirectory.MAC_APPLICATIONS,
            `Fractal/${shortcut}.app`
        )

        fs.rmdirSync(macDesktopPath, { recursive: true })
        fs.rmdirSync(macApplicationsPath, { recursive: true })
        return true
    }
    if (platform === OperatingSystem.WINDOWS) {
        // Points to the folder where windows.vbs is located (shortcut creation code)
        // Check the desktop folder and Start Menu Programs folder
        const windowsDesktopPath = path.join(
            FractalDirectory.DESKTOP,
            `${shortcut}.lnk`
        )
        const windowsStartMenuPath = path.join(
            FractalDirectory.WINDOWS_START_MENU,
            `Fractal/${shortcut}.lnk`
        )

        fs.unlinkSync(windowsDesktopPath)
        fs.unlinkSync(windowsStartMenuPath)
        return true
    }
    debugLog(`no suitable os found, instead got ${platform}`)
    return false
}
