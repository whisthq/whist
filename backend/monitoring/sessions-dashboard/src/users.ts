import { ManagementClient } from "auth0"
import { Auth0ClientID, Auth0ClientSecret, DeployEnvironment } from "./config"

const management = new ManagementClient({
  domain: `fractal-${DeployEnvironment}.us.auth0.com`,
  clientId: Auth0ClientID,
  clientSecret: Auth0ClientSecret,
})

export const getUser = (id: string) => management.getUser({ id })
