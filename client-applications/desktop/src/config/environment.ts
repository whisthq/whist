import { app } from "electron"

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
        `Got an unrecognized DEVELOPMENT_ENV: ${devEnv}. Defaulting to ${FractalEnvironments.LOCAL}`
      )
      return configs.LOCAL
  }
}

const getProductionConfig = () => {
  if (!app.isPackaged) {
    return configs.PRODUCTION
  }

  switch (appEnvironment) {
    case FractalEnvironments.DEVELOPMENT:
      return configs.DEVELOPMENT
    case FractalEnvironments.STAGING:
      return configs.STAGING
    case FractalEnvironments.PRODUCTION:
      return configs.PRODUCTION
    default:
      return configs.PRODUCTION
  }
}

export const config =
  process.env.NODE_ENV === FractalNodeEnvironments.DEVELOPMENT
    ? getDevelopmentConfig()
    : getProductionConfig()

export default config

// Re-exporting
export { FractalEnvironments } from "../../config/constants"
export { getLoggingBaseFilePath, loggingFiles } from "../../config/paths"
export const COMMIT_SHA = process.env.COMMIT_SHA ?? ""
