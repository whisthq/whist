import {
  environment,
  FractalCIEnvironment,
  FractalNodeEnvironment,
} from "../../config/environment"
import env from "@app/utils/env"

const getDevelopmentEnv = () => {
  switch (process.env.DEVELOPMENT_ENV) {
    case FractalCIEnvironment.LOCAL:
      return environment.LOCAL
    default:
      return environment.DEVELOPMENT
  }
}

const getProductionEnv = () => {
  switch (env.PACKAGED_ENV) {
    case FractalCIEnvironment.DEVELOPMENT:
      return environment.DEVELOPMENT
    case FractalCIEnvironment.STAGING:
      return environment.STAGING
    case FractalCIEnvironment.PRODUCTION:
      return environment.PRODUCTION
    default:
      return environment.PRODUCTION
  }
}

export const config =
  process.env.NODE_ENV === FractalNodeEnvironment.DEVELOPMENT
    ? getDevelopmentEnv()
    : getProductionEnv()

export default config

// Re-exporting
export { FractalCIEnvironment } from "../../config/environment"
export { auth0Config } from "../../config/environment"
export { loggingBaseFilePath } from "../../config/paths"
