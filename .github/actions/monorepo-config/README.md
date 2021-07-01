# Monorepo Config Action

This folder contains the code for the GitHub Action for the monorepo configuration builder. The program is written in Python, and deployed to GitHub Actions as a Docker container.

The program has a CLI built with the `click` library. All options are documented with the `--help` flag. `main.py` is the entrypoint to the program. After `pip install -r requirements.txt`, you can `python main.py --help` to see how run the program.

## How config works

All the common monorepo configuration is stored in the top-level `/config` folder. We use YAML for our configuration schema, with the schema files located in `/config/schema`. These YAML files are parsed by this program, and transformed into a JSON string containing a single flattened dictionary.

The schema files can use nested keys to define "profiles". These profiles are used to store values that need to change based on the environment, such as operating system or git branch. The list of profiles in use is stored in `config/profile.yml`.

When the configuration program is run inside GitHub Actions, it is passed the "secrets" that GitHub stores for us. It merges these secrets into the final configuration. Multiple sources can be set up for secrets, so that we can pull from other providers, like Auth0 or Heroku.

The processing of the configuration YAML is described below.

1. The program is passed profiles and secrets through command line arguments or environment variables.
2. All YAML files are read from `/config/schema`, parsed as nested dictionaries.
3. The schema dictionaries are merged by top-level key into a single dictionary. The merging is shallow.
4. The merged schema dictionary is flattened using the profiles passed as arguments to select nested keys.
5. The flattened schema dictionary is merged with the secrets passed as arguements. Secrets keys will override schema keys.
6. The final dictionary is written to a file or to stdout if no file argyment is passed.

The output JSON object, if processed correctly, should have no nesting. Nested configuration will remain in the output if no profile is found that matches a key. This is purposefully done so that the mistake is easy to spot.

## Example program run

For GitHub Actions reasons, it's good to get in the habit of running this program (and any Actions program) from the root folder of the monorepo. We'll run from the `fractal` folder like this:

```sh
python .github/actions/monorepo-config/main.py \
        --profile mac \
        --profile dev \
        --secrets '{"APPLE_API_KEY_ID": "a-really-secret-value"}'

```

This will output the JSON below. Excuse the null values for now, the configuration is still being populated. Note the `dev` specific URLs in keys like `AUTH_DOMAIN`, and `mac` specific URLS in keys like `CLIENT_DOWNLOAD`. These would be replaced with `win`, `lnx`, `local`, `staging`, or `production` URLS with different profiles.

```json
{
  "APPLE_API_KEY_ID": "a-really-secret-value",
  "APPLE_API_KEY_ISSUER_ID": null,
  "APPLE_OAUTH_SECRET": null,
  "AUTH0_GHA_CLIENT_SECRET_DEV": null,
  "AUTH0_GHA_CLIENT_SECRET_PROD": null,
  "AUTH0_GHA_CLIENT_SECRET_STAGING": null,
  "AUTH0_REFRESH_TOKEN_DEV": null,
  "AUTH_CLIENT_ID": "j1toifpKVu5WA6YbhEqBnMWN0fFxqw5I",
  "AUTH_DOMAIN": "fractal-dev.us.auth0.com",
  "AUTH_IDENTIFIER_API": "https://api.fractal.co",
  "AWS_EC2_ACCESS_KEY_ID": null,
  "AWS_EC2_S3_ACCESS_KEY_ID": null,
  "AWS_EC2_S3_SECRET_ACCESS_KEY_ID": null,
  "AWS_ECS_ACCESS_KEY_ID": null,
  "AWS_ECS_SECRET_ACCESS_KEY": null,
  "AWS_S3_ACCESS_KEY_ID": null,
  "AWS_S3_SECRET_ACCESS_KEY": null,
  "CLIENTAPP_AMPLITUDE_KEY": null,
  "CLIENTAPP_AWS_ACCESS_KEY": null,
  "CLIENTAPP_AWS_SECRET_KEY": null,
  "CLIENT_DOWNLOAD": "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
  "CLIENT_LOG_FILE_NAME": "client.log",
  "CLIENT_LOG_FOLDERNAME": "logs",
  "CLIENT_PERSISTENCE_FOLDER_NAME": "fractal",
  "ENV": "dev",
  "EXECUTABLE_NAME": "Fractal (development)",
  "FRONTEND": "dev.fractal.co",
  "GHA_PERSONAL_ACCESS_TOKEN": null,
  "GHA_USERNAME": null,
  "GOOGLE_OAUTH_SECRET": null,
  "HASURA_GRAPHQL_ACCESS_KEY_CONFIG": null,
  "HASURA_GRAPHQL_ACCESS_KEY_DEV": null,
  "HASURA_GRAPHQL_ACCESS_KEY_PROD": null,
  "HASURA_GRAPHQL_ACCESS_KEY_STAGING": null,
  "HEROKU_ALL_WEBSERVERS_ADMIN_PASSWORD": null,
  "HEROKU_ALL_WEBSERVERS_ADMIN_USERNAME": null,
  "HEROKU_DEVELOPER_API_KEY": null,
  "HEROKU_DEVELOPER_LOGIN_EMAIL": null,
  "HEROKU_PIPELINE_NAME": null,
  "HOST_SERVICE_AND_WEBSERVER_AUTH_SECRET_DEV": null,
  "HOST_SERVICE_AND_WEBSERVER_AUTH_SECRET_PROD": null,
  "HOST_SERVICE_AND_WEBSERVER_AUTH_SECRET_STAGING": null,
  "HOST_SERVICE_LOGZIO_SHIPPING_TOKEN": null,
  "ICON_FILE_NAME": "icon_dev",
  "MACOS_SIGNING_CERTIFICATE": null,
  "MACOS_SIGNING_CERTIFICATE_PASSWORD": null,
  "NODEJS": "development",
  "PROTOCOL_FILE_NAME": "_Fractal",
  "PROTOCOL_FOLDER_PATH": "../../MacOS",
  "PROTOCOL_LOG_FILE_NAME": "protocol.log",
  "REACT_APP_SHA_SECRET_KEY": null,
  "SENTRY_AUTH_TOKEN": null,
  "SLACK_EC2_ALERTS_OAUTH_TOKEN": null,
  "SLACK_HOOKS_ENDPOINT": null,
  "WEBSERVER": "https://dev-server.fractal.co",
  "WEBSERVER_LOGZIO_SHIPPING_TOKEN": null
}
```
