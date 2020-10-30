export const execChmodUnix = (
    command: string,
    path: string,
    platform: string
) => {
    return new Promise((resolve, reject) => {
        if (platform !== "win32") {
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
