import path from 'path'
import fs from 'fs'
import { app } from 'electron'

const getEnv = () => {
  if (!app.isPackaged) return {}

  const envFile = path
    .join(app.getAppPath(), 'env.json')
    .replace('build/dist/main/', '')
    .replace('app.asar/', '')

  return JSON.parse(fs.readFileSync(envFile, 'utf-8'))
}

export default getEnv()
