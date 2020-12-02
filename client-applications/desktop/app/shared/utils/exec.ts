import { OperatingSystem } from "shared/enums/client"

export const execChmodUnix = (
    command: string,
    path: string,
    platform: string
) => {
    return new Promise((resolve, reject) => {
        if (platform !== OperatingSystem.WINDOWS) {
            const { exec } = require("child_process")
            exec(command, { cwd: path }, (error, stdout, stderr) => {
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
    return new Promise((resolve, reject) => {
        const { spawn } = require("child_process")
        const os = require("os")
        const platform = os.platform()
        var appRootDir = require("electron").remote.app.getAppPath()
        var executable = ""
        var path = ""

        if (platform === OperatingSystem.MAC) {
            path = appRootDir + "/binaries/"
            path = path.replace("/Resources/app.asar", "")
            path = path.replace("/app", "")
            executable = "./awsping_osx"
        } else if (platform === OperatingSystem.WINDOWS) {
            path = appRootDir + "\\binaries"
            path = path.replace("\\resources\\app.asar", "")
            path = path.replace("\\app", "")
            executable = "awsping_windows.exe"
        } else {
            console.log(`no suitable os found, instead got ${platform}`)
        }

        execChmodUnix("chmod +x awsping_osx", path, platform).then(() => {
            const regions = spawn(executable, ["-n", "3"], {
                cwd: path,
            }) // ping via TCP
            regions.stdout.setEncoding("utf8")

            regions.stdout.on("data", (data: string) => {
                // Gets the line with the closest AWS region, and replace all instances of multiple spaces with one space
                const line = data.split(/\r?\n/)[0].replace(/  +/g, " ")
                const items = line.split(" ")
                // In case data is split and sent separately, only use closest AWS region which has index of 0
                if (items[1] == "1.") {
                    const region = items[2].slice(1, -1)
                    resolve(region)
                    // console.log("Ping detected " + region.toString())
                } else {
                    reject()
                }
            })
        })
    })
}
