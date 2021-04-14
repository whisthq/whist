import path from "path"
import fs from "fs"
import { app } from "electron"

const getEnv = () => {
    if (app.isPackaged) {
        const envFile = path
        .join(app.getAppPath(), "env.json")
        .replace("build/dist/main/", "")
        .replace("Resources/app.asar/", "")

        const rawJSON = fs.readFileSync(envFile, "utf-8")
        return JSON.parse(rawJSON)
    } else {
        return {}
    }
}

const env = getEnv()

export default env