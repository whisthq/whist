const fs = require("fs")
const util = require("util")
const log_file = fs.createWriteStream(__dirname + "/debug.log", { flags: "w" })
const log_stdout = process.stdout

const formatUserID = (userID: string) => {
    return userID ? userID : "No user ID"
}

export const logInfo = (logs: string, userID = "") => {
    logs = `${formatUserID(userID)} | ${logs}`
    log_file.write(util.format(logs) + "\n")
    log_stdout.write(util.format(logs) + "\n")
}

logInfo(`Logs can be found at ${__dirname}`)