# Fractal Auth0 Rules

This repository contains the code and deployment scripts for Fractal's Auth0 Rules. Auth0 is the identity management provider that Fractal uses. It handles signup, login, and some aspects of user management. Auth0 Rules are small callback scripts that are be triggered by Auth0 when users log in successfully. They are deployed to and run on Auth0's own servers.

### A Note About Hooks

Auth0 Hooks are similar to Rules in that they are scripts that are triggered to run by certain Auth0-defined events. Unlike Rules, which are only configurable to run when a user logs in successfully, Hooks may run in response to various user lifecycle events such as pre- and post- registration or password change.

Currently, the functionality that Auth0 Hooks provide is not necessary for any of Fractal's use cases. More specifically, we do not currently have any functionality that we need to add at any of the Auth0 Hooks [Extensibility Points](https://auth0.com/docs/hooks/extensibility-points): Client Credentials Exchange, Pre-User Registration, Post-User Registration, Post-Change Password, Send Phone Message. The only callback functions we need to run should be triggered by successful user logins.

## Development

Make sure to run `yarn` in the root directory to install dependencies.

The code for each rule can be found in the `src/rules` directory. Each rule is a single anonymous function with whose signature is `function (user, context, callback)`. See the article entitled [Rules Anatomy Best Practices](https://auth0.com/docs/best-practices/rules-best-practices/rules-anatomy-best-practices) for more information about the structure of Auth0 Rules. Please take special note of the sentence that instructs developers **not** to add a semicolon after the function body's closing curly brace!

## tenant.yaml

The `tenant.yaml` represents the full configuration of an Auth0 tenant. We have a separate tenant for `dev`, `staging`, and `prod`. The configuration for all of these tenants should be the same, save for certain variables such as the API url (API_IDENTIFIER). These variables are stored in each tenant's respective config file (`config/dev.json`, `config/staging.json`, `config/prod.json`). Note that these variables are referenced in `tenant.yaml`, for example in this snippet:

```yaml
---
resourceServers:
    - name: Fractal API
      identifier: "##API_IDENTIFIER##"
      allow_offline_access: true
      enforce_policies: true
```

The Auth0 CLI will replace these with the variables defined in the config JSON at deploy-time. Thus, it's important to note that **the `tenant.yaml` file is the same for each of our three tenants**. Any tenant-specific configuration should be moved to its respective config json file.

If you make changes in the Auth0 UI, make sure to run `yarn update-tenant` to download your changes in the local `tenant.yaml`. Changes made in the UI should only be done on the dev tenant. You may notice that this pulls the dev configuration -- this is by design. The flow for updating Auth0 configuration across the tenants is, broadly:

-   Change Auth0 config in dev tenant (with web UI, API, etc.)
-   Pull the new `tenant.yml`, commit and push to `dev` branch on GitHub
-   As the changes get promoted to `staging` and `prod`, GitHub Actions will update the Auth0 tenants accordingly

This ensures that the `tenant.yaml` in source control is the definite source-of-truth for that branch's Auth0 configuration. It also helps by minimizing the differences between our dev and prod Auth0 tenants -- so that no issues will be introduced upon promoting `dev` to `staging` to `prod`.

Note that `yarn update-tenant` may make undesirable changes to `tenant.yaml` or add unnecessary files to your git working tree. Be sure to review all changes made by `yarn update-tenant` in order to ensure that you only commit the changes you intend to commit.

### Clients

The `tenant.yaml` contains a list of clients -- these may represent either user-facing applications (eg. the Fractal desktop application) or trusted "machine-to-machine" clients such as GitHub Actions or the Fractal webserver. To add a client, it's recommended to use the Auth0 web UI and updating `tenant.yaml` with `yarn update-tenant`.

Here's an example client configuration:

```yaml
- name: Webserver
  app_type: non_interactive
  cross_origin_auth: false
  custom_login_page_on: true
  grant_types:
      - client_credentials
  is_first_party: true
  is_token_endpoint_ip_header_trusted: false
  jwt_configuration:
      alg: RS256
      lifetime_in_seconds: 36000
      secret_encoded: false
  oidc_conformant: true
  refresh_token:
      expiration_type: non-expiring
      leeway: 0
      infinite_token_lifetime: true
      infinite_idle_token_lifetime: true
      token_lifetime: 31557600
      idle_token_lifetime: 2592000
      rotation_type: non-rotating
  sso_disabled: false
  token_endpoint_auth_method: client_secret_post
```

Many of these fields are self-explanatory, but some require more explanation:

-   `app_type` indicates the type of this app. "non_interactive" means that this is a machine-to-machine client, "native" would be for the Fractal desktop application, etc.
-   `jwt_configuration.alg` is the algorithm used to sign access tokens for this client. This should be 'RS256' (asymmetric signing algorithm, can be verified with a public key) unless there's a very good reason otherwise.
-   `jwt_configuration.lifetime_in_seconds` is the lifetime (in seconds) of an _access_ token. By default, this is 1 day
-   `refresh_token.token_lifetime` is the _maximum_ lifetime (in seconds) of a _refresh_ token
-   `refresh_token.idle_token_lifetime` is the length of time (in seconds) that a refresh token may be not used before the token gets invalidated.

## Adding a New Social Provider

Auth0 supports the addition of new social providers. For a comprehensive list and specific instructions, you can visit [this page](https://auth0.com/docs/connections/identity-providers-social).

Integrating a social provider with Auth0 includes generating an OAuth secret and supplying that to Auth0. If we're not careful, that secret can 1) leak into tenant.yaml, and 2) be overwritten in Auth0 when deploying a tenant configuration.

To address these issues, a "censorMapping" is used in `scripts/update-tenant.js` that maps sensitive paths to the names of environment variables that they should be replaced with. For example, Apple's OAuth secret is replaced with "##APPLE_OAUTH_SECRET##". This value is replaced with the environment variable APPLE_OAUTH_SECRET at deploy-time. Thus, it is also important to include the `XYZ_OAUTH_SECRET`s in Github Actions (or whatever environment the `yarn deploy:[env]` commands are invoked in)

### Including Dependencies

Auth0 Rules are executed in an environment in which a subset of all versions of a subset of all public NPM modules are available to be required. Visit https://auth0-extensions.github.io/canirequire/ to search for NPM modules that are available to be required by your Auth0 Rules.

Since each rule is a a single JavaScript function, note that dependency modules should be required inside the function's body. Requiring modules outside of the function's body would probably prevent the Hook code from being parsed correctly.

```javascript
function (user, context, callback) {
    const stripe = require("stripe@8.126.0")(configuration.STRIPE_API_KEY)
    ...
}
```

## Deploying

The `yarn deploy:[dev|staging|prod]` command will deploy all Rules to the specified tenant. GitHub Actions will do this for you in our deployment workflow.
