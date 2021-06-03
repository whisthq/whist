# Fractal Auth0 Hooks

This repository contains the code -- plus some build and deploy scripts -- for Fractal's Auth0 hooks, which are small microservices triggered by Auth0 when certain events occur (user registration, password change, etc.). Auth0 is the identity management provider that Fractal uses, and handles sign-in, registration, and some aspects of user management. Hooks are deployed and run on Auth0's own servers.

## Development

Make sure to run `yarn` in the root directory to install dependencies.

The code for each hook can be found in the `src` directory. Each hook should export a function with the signature `function (user, context, callback)`. Details regarding specific hook types can be found in [the Auth0 documentation](https://auth0.com/docs/hooks/extensibility-points). Note that the manner in which hooks are run can differ depending on the type: some hooks are blocking, some expect a response, others are non-blocking "side effects".

## tenant.yaml

tenant.yaml represents the full configuration of an auth0 tenant. We have a separate tenant for `dev`, `staging`, and `prod`. The configuration for all of these tenants should be the same, save for certain variables such as the API url (API_IDENTIFIER). These variables are stored in each tenant's respective config file (`config/dev.json`, `config/staging.json`, `config/prod.json`). Note that these variables are referenced in tenant.yaml, for example in this snippet:

```yaml
---
resourceServers:
  - name: Fractal API
    identifier: "##API_IDENTIFIER##"
    allow_offline_access: true
    enforce_policies: true
```

The Auth0 cli will replace these with the variables defined in the config json at deploy-time. Thus, it's important to note that **the tenant.yaml file is the same for each of our three tenants**. Any tenant-specific configuration should be moved to its respective config json file.

If you make changes in the Auth0 UI, make sure to run `yarn update-tenant` to download your changes in the local `tenant.yaml`. Changes made in the UI should only be done on the dev tenant. You may notice that this pulls the dev configuration -- this is by design. The flow for updating Auth0 configuration across the tenants is, broadly:

- Change Auth0 config in dev tenant (with web UI, API, etc.)
- Pull the new tenant.yml, commit and push to dev branch on github
- As the changes get promoted to staging and prod, github actions will update the Auth0 tenants accordingly

This ensures that the `tenant.yaml` in source control is the definite source-of-truth for that branch's Auth0 configuration. It also helps by minimizing the differences between our dev and prod Auth0 tenants -- so that no issues will be introduced upon promoting dev to staging to prod.

## Adding a new social provider

Auth0 supports the addition of new social providers. For a comprehensive list and specific instructions, you can visit [this page](https://auth0.com/docs/connections/identity-providers-social).

Integrating a social provider with Auth0 includes generating an OAuth secret and supplying that to Auth0. If we're not careful, that secret can 1) leak into tenant.yaml, and 2) be overwritten in Auth0 when deploying a tenant configuration.

To address these issues, a "censorMapping" is used in `scripts/update-tenant.js` that maps sensitive paths to the names of environment variables that they should be replaced with. For example, Apple's OAuth secret is replaced with "##APPLE_OAUTH_SECRET##". This value is replaced with the environment variable APPLE_OAUTH_SECRET at deploy-time. Thus, it is also important to include the `XYZ_OAUTH_SECRET`s in Github Actions (or whatever environment the `yarn deploy:[env]` commands are invoked in)

## Building

To build all hooks, run `yarn build` in the project's root directory. You can then add the hook to Auth0 by modifying tenant.yaml. Here's an example of a hook:

```yaml
- name: post-user-registration-hook
  script: ./hooks/post-user-registration-hook.js
  dependencies:
    stripe: 8.145.0
    lodash: 4.17.21
    node-fetch: 2.6.1
    browser-or-node: 1.3.0
  enabled: true
  secrets:
    STRIPE_KEY: _VALUE_NOT_SHOWN_
    STRIPE_PRICE_ID: _VALUE_NOT_SHOWN_
  triggerId: post-user-registration
```

Make sure that the `script:` field points to the bundled file (in `dist`).

### Including dependencies

Due to limitations that Auth0 sets on the filesize of hooks, a fully-bundled, self-contained hook file cannot be used.

We sidestep this constraint by not including public npm dependencies (eg. stripe, lodash). These can be installed to the hook by modifying tenant.yaml. In the example above, these are the dependencies.

```yaml
dependencies:
  stripe: 8.145.0
  lodash: 4.17.21
  node-fetch: 2.6.1
  browser-or-node: 1.3.0
```

You can run `yarn list-dependencies` (eg. `yarn list dist/post-user-registration-hook.js`) on a bundled hook file to generate a list of dependencies for that hook.

## Deploying

The `yarn deploy:[dev|staging|prod]` command will deploy all hooks to the specified tenant. Github actions will do this for you.
