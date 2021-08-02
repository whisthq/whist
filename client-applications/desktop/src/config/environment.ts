import { app } from "electron"
import path from "path"

import {
  appEnvironment,
  configs,
  FractalEnvironments,
  FractalNodeEnvironments,
} from "../../config/configs"

const getDevelopmentConfig = () => {
  const devEnv = process.env.DEVELOPMENT_ENV as string
  switch (devEnv) {
    case FractalEnvironments.LOCAL:
      return configs.LOCAL
    case FractalEnvironments.DEVELOPMENT:
      return configs.DEVELOPMENT
    case FractalEnvironments.STAGING:
      return configs.STAGING
    case FractalEnvironments.PRODUCTION:
      return configs.PRODUCTION
    default:
      console.warn(
        `Got an unrecognized DEVELOPMENT_ENV: ${devEnv}. Defaulting to ${FractalEnvironments.DEVELOPMENT}`
      )
      return configs.DEVELOPMENT
  }
}

const getProductionConfig = () => {
  if (!app.isPackaged) {
    return configs.DEVELOPMENT
  }

  switch (appEnvironment) {
    case FractalEnvironments.LOCAL:
      return configs.LOCAL
    case FractalEnvironments.DEVELOPMENT:
      return configs.DEVELOPMENT
    case FractalEnvironments.STAGING:
      return configs.STAGING
    case FractalEnvironments.PRODUCTION:
      return configs.PRODUCTION
    default:
      return configs.DEVELOPMENT
  }
}

export const config =
  process.env.NODE_ENV === FractalNodeEnvironments.DEVELOPMENT
    ? getDevelopmentConfig()
    : getProductionConfig()

const { deployEnv } = config
const appPath = app.getPath("userData")

export const loggingBaseFilePath = path.join(appPath, deployEnv)

// Re-exporting
export { FractalEnvironments } from "../../config/constants"
export { loggingFiles } from "../../config/paths"

export default config
