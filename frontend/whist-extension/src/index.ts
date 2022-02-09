import { config } from "@app/constants/app"

export const authPortalURL = () => {
  const parsedConfig = JSON.parse(JSON.stringify(config))
  return [
    `https://${parsedConfig.AUTH_DOMAIN_URL}/authorize`,
    `?audience=${parsedConfig.AUTH_API_IDENTIFIER}`,
    // We need to request the admin scope here. If the user is enabled for the admin scope
    // (e.g. marked as a developer in Auth0), then the returned JWT token will be granted
    // the admin scope, thus bypassing Stripe checks and granting additional privileges in
    // the webserver. If the user is not enabled for the admin scope, then the JWT token
    // will be generated but will not have the admin scope, as documented by Auth0 in
    // https://auth0.com/docs/scopes#requested-scopes-versus-granted-scopes
    "&scope=openid profile offline_access email admin",
    "&response_type=code",
    `&client_id=${parsedConfig.AUTH_CLIENT_ID}`,
    `&redirect_uri=${parsedConfig.CLIENT_CALLBACK_URL}`,
  ].join("")
}

chrome.tabs.create({
  url: chrome.runtime.getURL("src/newtab.html"),
})
