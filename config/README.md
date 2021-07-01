# Fractal Monorepo Config

This folder contains common configuration values that can be re-used across our monorepo.

## How config works

All the common monorepo configuration is stored in the top-level `/config` folder. We use YAML for our configuration schema, with the schema files located in `/config/schema`. These YAML files are parsed by this program, and transformed into a JSON string containing a single flattened dictionary.

When the configuration program is run inside GitHub Actions, it is passed the "secrets" that GitHub stores for us. It merges these secrets into the final configuration. Multiple sources can be set up for secrets, so that we can pull from other providers, like Auth0 or Heroku.

The schema files can use nested keys to define "profiles". These profiles are used to store values that need to change based on the environment, such as operating system or git branch. The list of profiles in use is stored in `config/profile.yml`.

"Profiles" refers to one of the sets of keys in "config/profiles.yml". These are used to choose between values like "macos/win32/linux". With a profile of "macos", a config like this:

```json
{
  "PROTOCOL_FILE_NAME": {
    "macos": "_Fractal",
    "win32": "Fractal.exe",
    "linux": "Fractal"
  }
}
```

...gets flattened to this:

```json
{
  "PROTOCOL_FILE_NAME": "_Fractal"
}
```

This allows you to pass only the necessary configuration values in your application build. Profiles are optional, and if you don't pass them then your build will receive the original nested data structure.

## Example program run

For GitHub Actions reasons, it's good to get in the habit of running this program (and any Actions program) from the root folder of the monorepo. We'll run from the `fractal` folder like this:

```sh
python .github/actions/monorepo-config/main.py \
        --path config \
        --secrets '{"APPLE_API_KEY_ID": "a-really-secret-value"}'
        --deploy dev \
        --os macos \

```

Here, we're passing `--secrets` as JSON, and we can expect those values to be merged into the final config. Note that that secret keys (like `APPLE_API_KEY_ID` above), must _already be present_ in one of the `.yml` schema files for the merge to be successful. The value can be empty, or something like "\*\*\*\*\*\*\*". We do this because it's important for config schemas to serve as documentation for our secret CI values. By keeping (and commenting) the secret keys in the `.yml` files, they remain visible to the whole team.

The `--deploy` and `--os` flags are examples of "profiles". Each profile name must be present in `profile.yml` for the command to succeed.

Depending on the data in the YAML files, this might output JSON like you see below. Note the `dev` specific URLs in keys like `AUTH_DOMAIN`, and `macos` specific URLS in keys like `CLIENT_DOWNLOAD`. These would be replaced with `win32`, `linux`, `local`, `staging`, or `production` URLS with different profiles.

```json
{
  "APPLE_API_KEY_ID": "a-really-secret-value",
  "AUTH_CLIENT_ID": "j1toifpKVu5WA6YbhEqBnMWN0fFxqw5I",
  "AUTH_DOMAIN": "fractal-dev.us.auth0.com",
  "AUTH_IDENTIFIER_API": "https://api.fractal.co",
  "CLIENT_DOWNLOAD": "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
  "CLIENT_LOG_FILE_NAME": "client.log",
  "CLIENT_LOG_FOLDERNAME": "logs",
  "CLIENT_PERSISTENCE_FOLDER_NAME": "fractal",
  "ENV": "dev",
  "EXECUTABLE_NAME": "Fractal (development)",
  "FRONTEND": "dev.fractal.co",
  "ICON_FILE_NAME": "icon_dev",
  "NODEJS": "development",
  "PROTOCOL_FILE_NAME": "_Fractal",
  "PROTOCOL_FOLDER_PATH": "../../MacOS",
  "PROTOCOL_LOG_FILE_NAME": "protocol.log",
  "WEBSERVER": "https://dev-server.fractal.co"
}
```
