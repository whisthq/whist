import { OperatingSystem, FractalDirectory } from "shared/types/client"
import { ChildProcess } from "child_process"

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
        Helper function for launchProtocol(). Gets the Fractal client protocol executable name
        depending on the user's operating system

    Arguments: none

    Returns:
        (string) name of executable to run

    */
    const currentOS = require("os").platform()

    if (currentOS === OperatingSystem.MAC) {
        return "./Fractal"
    }
    if (currentOS === OperatingSystem.WINDOWS) {
        return "Fractal.exe"
    }
    return ""
}

export const launchProtocol = async (
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

    Returns:
        Child process object created by spawn()
    */

    // spawn launches an executable in a separate thread
    const spawn = require("child_process").spawn
    // Get the path and name of the protocol in the packaged app
    const protocolPath = require("path").join(
        FractalDirectory.getRootDirectory(),
        require("os").platform() === OperatingSystem.MAC
            ? "MacOS"
            : "protocol-build/desktop"
    )
    const iconPath = require("path").join(
        FractalDirectory.getRootDirectory(),
        "build/icon64.png"
    )

    const executable = getExecutableName()

    // On Mac, run chmod to allow the protocol to run
    await execPromise("chmod +x Fractal", protocolPath, [
        OperatingSystem.MAC,
        OperatingSystem.LINUX,
    ])

    // Protocol arguments
    const protocolParameters = {
        name: "Fractalized Chrome",
        icon: iconPath,
    }

    // TODO: Make the protocol start without an IP address
    const protocolArguments = [
        ...Object.entries(protocolParameters)
            .map(([flag, arg]) => [`--${flag}`, arg])
            .flat(),
        "--read-pipe",
    ]

    // Starts the protocol
    protocolOnStart()
    const protocol = spawn(executable, protocolArguments, {
        cwd: protocolPath,
        detached: false,
        stdio: ["pipe", process.stdout, process.stderr],
    })

    // On protocol exit logic, fired only when protocol stops running
    protocol.on("close", () => {
        protocolOnExit()
    })

    return protocol
}

export const writeStream = (
    process: ChildProcess | undefined,
    message: string
): boolean => {
    if (process && process.stdin) {
        process.stdin.write(message)
        process.stdin.write("\n")
        return true
    }
    return false
}

export const endStream = (
    process: ChildProcess | undefined,
    message: string
): boolean => {
    if (process && process.stdin) {
        process.stdin.end(message)
        return true
    }
    return false
}
