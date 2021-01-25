import { OperatingSystem, FractalDirectory } from "shared/types/client"
import { Container } from "store/reducers/container/default"

export const execPromise = (
    command: string,
    path: string,
    targetOS: OperatingSystem[]
) => {
    /*
    Description:
        Executes a shell command on target operating systems

    Arguments:
        command (string) : Shell command (e.g. echo "Hello world")
        path (string) : Absolute path for working directory in which to run shell command
        targetOS (OperatingSystem[]) : Operating systems on which to run this command

    Returns:
        promise : Promise
    */

    const clientOS = require("os").platform()
    return new Promise((resolve, reject) => {
        // If not on a target OS, run shell command, otherwise do nothing
        if (targetOS.includes(clientOS)) {
            const { exec } = require("child_process")
            exec(command, { cwd: path }, (error, stdout) => {
                if (error) {
                    reject(error)
                    return
                }
                resolve(stdout)
            })
        } else {
            resolve()
        }
    })
}

const getExecutableName = (): string => {
    /*
    Description:
        Helper function for launchProtocol(). Gets the FractalClient executable name
        depending on the user's operating system

    Arguments: none

    Returns: 
        (string) name of executable to run
        
    */
    const currentOS = require("os").platform()

    if (currentOS === OperatingSystem.MAC) {
        return "./FractalClient"
    }
    if (currentOS === OperatingSystem.WINDOWS) {
        return "FractalClient.exe"
    }
    return ""
}

export const launchProtocol = (
    container: Container,
    protocolOnStart: () => void,
    protocolOnExit: () => void
) => {
    /*
    Description:
        Function to launch the protocol

    Arguments:  
        container: Container from Redux store
        protocolOnStart (function): Callback function fired right before protocol starts
        protocolOnExit (function): Callback function fired right after protocol exits
    */

    // spawn launches an executable in a separate thread
    const spawn = require("child_process").spawn
    // Get the path and name of the protocol in the packaged app
    const protocolPath = require("path").join(
        FractalDirectory.getRootDirectory(),
        "protocol-build/desktop"
    )
    const iconPath = require("path").join(
        FractalDirectory.getRootDirectory(),
        "build/icon64.png"
    )

    const executable = getExecutableName()
    // On Mac, run chmod to allow the protocol to run
    execPromise("chmod +x FractalClient", protocolPath, [
        OperatingSystem.MAC,
        OperatingSystem.LINUX,
    ])
        .then(() => {
            // Protocol arguments
            const portInfo = `32262:${container.port32262}.32263:${container.port32263}.32273:${container.port32273}`
            const protocolParameters = {
                width: window.screen.availWidth * window.devicePixelRatio,
                height: window.screen.availHeight * window.devicePixelRatio,
                ports: portInfo,
                "private-key": container.secretKey,
                name: "Fractalized Chrome",
                icon: iconPath,
            }

            const protocolArguments = [
                ...Object.entries(protocolParameters)
                    .map(([flag, arg]) => [`--${flag}`, arg])
                    .flat(),
                container.publicIP,
            ]

            // Starts the protocol
            protocolOnStart()
            const protocol = spawn(executable, protocolArguments, {
                cwd: protocolPath,
                detached: false,
                stdio: "ignore",
            })

            // On protocol exit logic, fired only when protocol stops running
            protocol.on("close", () => {
                protocolOnExit()
            })
            return null
        })
        .catch((error) => {
            throw error
        })
}
