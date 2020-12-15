import { OperatingSystem, FractalDirectory } from "shared/types/client"
import { debugLog } from "shared/utils/general/logging"
import { allowedRegions } from "shared/types/aws"

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

export const setAWSRegion = () => {
    /*
    Description:
        Runs AWS ping shell script (tells us which AWS server is closest to the client)

    Arguments:
    
    Returns:
        promise : Promise
    */
    return new Promise((resolve) => {
        const { spawn } = require("child_process")
        const platform = require("os").platform()
        const path = require("path")
        const fs = require("fs")

        let executableName
        const binariesPath = path.join(
            FractalDirectory.getRootDirectory(),
            "binaries"
        )

        if (platform === OperatingSystem.MAC) {
            executableName = "awsping_osx"
        } else if (platform === OperatingSystem.WINDOWS) {
            executableName = "awsping_windows.exe"
        } else if (platform == OperatingSystem.LINUX) {
            executableName = "awsping_linux"
        } else {
            debugLog(`no suitable os found, instead got ${platform}`)
        }

        const executablePath = path.join(binariesPath, executableName)

        fs.chmodSync(executablePath, 0o755)

        const regions = spawn(executablePath, ["-n", "3"]) // ping via TCP
        regions.stdout.setEncoding("utf8")

        regions.stdout.on("data", (data: string) => {
            // Gets the line with the closest AWS region, and replace all instances of multiple spaces with one space
            const output = data.split(/\r?\n/)
            let index = 0
            let line = output[index].replace(/  +/g, " ")
            let items = line.split(" ")
            while (
                !allowedRegions.includes(items[2].slice(1, -1)) &&
                index < output.length - 1
            ) {
                index += 1
                line = output[index].replace(/  +/g, " ")
                items = line.split(" ")
            }
            // In case data is split and sent separately, only use closest AWS region which has index of 0
            const region = items[2].slice(1, -1)
            resolve(region)
        })
        return null
    })
}
