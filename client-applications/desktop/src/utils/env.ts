import path from "path"
import fs from "fs"
import { app } from "electron"

const getEnv = () => {
  if (!app.isPackaged) return {}

  // If packaged, then app.getAppPath() points to app.asar
  const envFile = path.join(app.getAppPath(), "..", "env.json")

  return JSON.parse(fs.readFileSync(envFile, "utf-8"))
}

export default getEnv()
