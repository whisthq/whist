import { app } from "electron"
import path from "path"

import {
  appEnvironment,
  configs,
  WhistEnvironments,
  WhistNodeEnvironments,
} from "../../config/configs"

const getDevelopmentConfig = () => {
  const devEnv = process.env.DEVELOPMENT_ENV as string
  switch (devEnv) {
    case WhistEnvironments.LOCAL:
      return configs.LOCAL
    case WhistEnvironments.DEVELOPMENT:
      return configs.DEVELOPMENT
    case WhistEnvironments.STAGING:
      return configs.STAGING
    case WhistEnvironments.PRODUCTION:
      return configs.PRODUCTION
    default:
      console.warn(
        `Got an unrecognized DEVELOPMENT_ENV: ${devEnv}. Defaulting to ${WhistEnvironments.DEVELOPMENT}`
      )
      return configs.DEVELOPMENT
  }
}

const getProductionConfig = () => {
  if (!app.isPackaged) {
    return configs.LOCAL
  }

  switch (appEnvironment) {
    case WhistEnvironments.LOCAL:
      return configs.LOCAL
    case WhistEnvironments.DEVELOPMENT:
      return configs.DEVELOPMENT
    case WhistEnvironments.STAGING:
      return configs.STAGING
    case WhistEnvironments.PRODUCTION:
      return configs.PRODUCTION
    default:
      return configs.DEVELOPMENT
  }
}

export const config =
  process.env.NODE_ENV === WhistNodeEnvironments.DEVELOPMENT
    ? getDevelopmentConfig()
    : getProductionConfig()

const { deployEnv } = config
const appPath = app.getPath("userData")

export const loggingBaseFilePath = path.join(appPath, deployEnv)

// Re-exporting
export { WhistEnvironments } from "../../config/constants"
export { loggingFiles } from "../../config/paths"

export default config
