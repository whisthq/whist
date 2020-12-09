import { OperatingSystem } from "shared/types/client"
import { debugLog } from "shared/utils/general/logging"

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
    return new Promise((resolve, reject) => {
        const { spawn } = require("child_process")
        const os = require("os")
        const platform = os.platform()
        const appRootDir = require("electron").remote.app.getAppPath()

        let executable = ""
        let path = ""

        if (platform === OperatingSystem.MAC) {
            path = `${appRootDir}/binaries/`
            path = path.replace("/Resources/app.asar", "")
            path = path.replace("/app", "")
            executable = "./awsping_osx"
        } else if (platform === OperatingSystem.WINDOWS) {
            path = `${appRootDir}\\binaries`
            path = path.replace("\\resources\\app.asar", "")
            path = path.replace("\\app", "")
            executable = "awsping_windows.exe"
        } else {
            debugLog(`no suitable os found, instead got ${platform}`)
        }

        execPromise("chmod +x awsping_osx", path, [
            OperatingSystem.MAC,
            OperatingSystem.LINUX,
        ])
            .then(() => {
                const regions = spawn(executable, ["-n", "3"], {
                    cwd: path,
                }) // ping via TCP
                regions.stdout.setEncoding("utf8")

                regions.stdout.on("data", (data: string) => {
                    // Gets the line with the closest AWS region, and replace all instances of multiple spaces with one space
                    const line = data.split(/\r?\n/)[0].replace(/  +/g, " ")
                    const items = line.split(" ")
                    // In case data is split and sent separately, only use closest AWS region which has index of 0
                    if (items[1] === "1.") {
                        const region = items[2].slice(1, -1)
                        resolve(region)
                        // debugLog("Ping detected " + region.toString())
                    } else {
                        reject()
                    }
                })
                return null
            })
            .catch((err) => {
                throw err
            })
    })
}
