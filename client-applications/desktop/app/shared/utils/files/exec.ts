import { ChildProcess } from "child_process"
import { OperatingSystem, FractalDirectory } from "../../types/client"

// TODO: REVERT ALL EXEUTABLE CHANGES TO MATCH
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
        return process.env.NODE_ENV === "development"
            ? "./FractalClient"
            : "./Fractal"
    }
    if (currentOS === OperatingSystem.WINDOWS) {
        return "Fractal.exe"
    }
    return ""
}

/**
 * Launches the protocol
 * @param protocolOnStart function called right before protocol starts
 * @param protocolOnExit function called right after protocl exits
 * @param userID userID for logging
 * @param protocolLaunched protoclLaunched timestamp for logging
 * @param createContainerRequestSent container request timestamp for logging
 *
 * Returns: child process object created by spawn
 */
export const launchProtocol = async (
    protocolOnStart: (userID: string) => void,
    protocolOnExit: (
        protocolLaunched: number,
        createContainerRequestSent: number,
        userID: string,
        code: number,
        signal: string
    ) => void,
    userID: string,
    protocolLaunched: number,
    createContainerRequestSent: number
) => {
    // spawn launches an executable in a separate thread
    const spawn = require("child_process").spawn

    // Get the path and name of the protocol in the packaged app
    // TODO: add conditino for mac os path directory

    const protocolPath = require("path").join(
        FractalDirectory.getRootDirectory(),
        require("os").platform() === OperatingSystem.MAC &&
            process.env.NODE_ENV !== "development"
            ? "MacOS"
            : "protocol-build/client"
    )

    const iconPath = require("path").join(
        FractalDirectory.getRootDirectory(),
        "build/icon64.png"
    )

    const executable = getExecutableName()
    const command =
        process.env.NODE_ENV === "development"
            ? "chmod +x FractalClient"
            : "chmod +x Fractal"
    // On Mac, run chmod to allow the protocol to run
    await execPromise(command, protocolPath, [
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
    protocolOnStart(userID)

    const protocol = spawn(executable, protocolArguments, {
        cwd: protocolPath,
        detached: false,
        stdio: ["pipe", process.stdout, process.stderr],
    })

    // On protocol exit logic, fired only when protocol stops running
    protocol.on("close", (code: number, signal: string) => {
        protocolOnExit(
            protocolLaunched,
            createContainerRequestSent,
            userID,
            code,
            signal
        )
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
