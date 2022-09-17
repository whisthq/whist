export const DeployEnvironment =
  process.env["npm_config_deploy_environment"] ?? "prod"
export const Auth0ClientSecret =
  process.env["npm_config_auth0_client_secret"] ?? ""
export const Auth0ClientID = process.env["npm_config_auth0_client_id"] ?? ""
export const HasuraAdminSecret =
  process.env["npm_config_hasura_admin_secret"] ?? ""
