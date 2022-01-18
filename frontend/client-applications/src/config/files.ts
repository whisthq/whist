import path from "path"
import { WhistEnvironments, appEnvironment } from "../../config/configs"
import config from "@app/config/environment"

const { buildRoot } = config

const iconPath = () => {
  if (appEnvironment === WhistEnvironments.PRODUCTION)
    return path.join(buildRoot, "icon.png")
  if (appEnvironment === WhistEnvironments.STAGING)
    return path.join(buildRoot, "icon_staging.png")
  if (appEnvironment === WhistEnvironments.DEVELOPMENT)
    return path.join(buildRoot, "icon_dev.png")

  return path.join(buildRoot, "icon.png")
}

export { iconPath }
