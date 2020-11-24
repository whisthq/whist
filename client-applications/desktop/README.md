# Fractal Desktop Applications

![Electron CI](https://github.com/fractal/client-applications/workflows/Electron%20CI/badge.svg)

This folder contains the code for the Fractal desktop applications running on Windows, MacOS and Linux Ubuntu. The applications are built with cross-platform compatibility using ElectronJS. This repository contains all the directions for building the applications locally and for publishing them for production on each of the following OSes:

-   Windows 10
-   MacOS 10.10+
-   Linux Ubuntu 18.04+

## Starting Development

Before starting development, you need to install all of the application and development dependencies by running `yarn` (which is a JavaScript package manager).

If you are still experiencing issues with starting the dev environment, you might need to run `yarn upgrade`, which will upgrade all the dependencies. It's a good idea to do so periodically to keep the application up-to-date. If further issues persist, you can try reinstalling by running `rm -rf node_modules/` then `yarn` again.

To start the application in the `dev` environment, run `yarn dev`. To start development with a custom port, run `yarn cross-env PORT={number} yarn dev`. This will start the Electron application, but will not fetch the Fractal protocol, which means you can only use this to test the application itself, unless you manually cloned and built the protocol yourself. If you're looking to test launching the Fractal protocol from the application, see **Packaging for Production** below.

This repository has continuous integration through GitHub Actions, which you can learn more about under [Continuous Integration](#Continuous Integration).

## Packaging and Publishing

The Fractal application is a combination of this `client-applications` codebase and the Fractal `protocol`. It's possible to run them in isolation for testing, but they must be tested and released as a combined package. The script `build_and_publish.py` is a single, cross-platform script that coordinates the creation and release of a combined executable.

Poetry is used to manage the dependencies and virtual environment for the build scripts. Follow [these instructions](https://python-poetry.org/docs/#installation) to install it. Next, run `poetry install` in the repository root to install the necessary dependencies. Then enable the virtual environment with `poetry shell`.

**To download the latest version of the protocol** run `python retrieve_protocol_packages.py`.

**To build the desktop application** run `python desktop/build_and_publish.py`.

### Retrieving the Protocol

The Fractal protocol is maintained in a separate repository and a release of it must be downloaded before you can proceed with building a final executable. The tool `retrieve_protocol_packages.py` has been created to facilitate the retrieval of pre-compiled releases of the Fractal protocol.

By default `retrieve_protocol_packages.py` will download the latest release on the `dev` branch, though it can be used to download any release. You can see the available functionality by running `python retrieve_protocol_packages.py --help`.

### Building the Installer Executable

To combine the protocol and this wrapper client application into a single installer executable you will use the `build_and_publish.py` tool.

By default `build_and_publish.py` will use the latest version of the protocol retrieved via `retrieve_protocol_packages.py` and will build an executable subscribed to the `testing` branch. You can see additional available functionality by running `python build_and_publish.py --help`.

The installer executable will be in `client-applications/desktop/release` as a `.dmg` (MacOS), `.exe` (Windows) or `.AppImage` (Linux Ubuntu). No cross-compilation is possible; for instance you can only package the Windows application from a Windows computer.

#### MacOS Notarizing

Before you can package the MacOS application it needs to be notarized. This means that it needs to be uploaded to Apple's servers and scanned for viruses and malware. This is all automated as part of Electron, although you need to have the Fractal Apple Developer Certificate in your MacOS Keychain for this work successfully. You can download the certificate from AWS S3 on [this link](https://fractal-private-dev.s3.amazonaws.com/fractal-apple-codesigning-certificate.p12) assuming you have access to the Fractal AWS organization, and then install it by double-clicking the `.p12` certificate file. The application will get notarized as part of the regular build script.

In order for this to work, you need to have installed the latest version of Xcode (which you can install from the macOS App Store), and have opened it *at least* once following installation, which will prompt you to install additional components. Once those components are installed, you need to open up a terminal and run `xcode-select --install` to install the Xcode CLI, which is necessary for the notarizing to work. Once all of these are done, you should have no problem with the notarization process as part of the packaging of the application.

### Publishing New Versions

Fractal runs two update channels, `production` and `testing`. The `dev` branch should be published automatically to `testing`, while `production` should match `master`. The build script has a special `noupdates` channel which should be used for any builds that aren't on one of these branches.

Any CI generated builds are also stored in GitHub Releases which can be manually downloaded and used.

CI should handle releases, however,

#### Update Channels

*The source of truth for these should be `build_and_publish.py` in the `default_channel_s3_buckets` dictionary.*

There is a channel for `testing` and `production` on each platform. These channels are backed by AWS S3 buckets ([here](https://s3.console.aws.amazon.com/s3/home?region=us-east-1#)) that follow a file structure and metadata schema specified by [electron-builder's publish system](https://www.electron.build/configuration/publish) (it's basically the executable installer + a well-known YAML file with metadata like file hash, file name, and release date which is used for knowing when an update is available).

#### Publishing to Production

If you're ready to publish an update to production, as part of our [Release Schedule](https://www.notion.so/tryfractal/Release-Schedule-Drafting-c29cbe11c5f94cedb9c01aaa6d0d1ca4), then it is time to publish. You can publish by running `python build_and_publish.py --set-version master-YYYYMMDD.# --update-channel production --push-new-update`.

**TODO: once the following manual fix is removed, CI should fully handle these release operations.**

First, go to `node_modules/builder-util-runtime/out/httpExecutor.js`, and change the timeout on Line 319 from `60 * 1000` to `60 * 1000 * 1000`. This is necessary to avoid timeout errors for connection in the production application.

## Continous Integration

This repository has basic continuous integration through GitHub Actions. For every PR to `dev`, `staging`, or `master`, GitHub Actions will attempt to build the bundled application on Windows-64bit, macOS-64bit, and Linux-64bit. It will upload these builds to the GitHub Releases tab with a version identifier corresponding to the current git ref (eg. branch) and the current date.

New builds from the `dev` will also be pushed out on the `testing` channel.

Moreover, new builds from [fractal/protocol](https://github.com/fractal/protocol) will trigger builds in this repository on their corresponding branch (or on `dev` if there is no appropriate corresponding branch). Similarly, new protocol builds on `dev` will also trigger a new build to be sent out on the `testing` channel.

Additionally, [style](#Styling) checks will be run to verify that you formatted your code via Prettier. You should make sure that all tests pass under the Actions tab.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io/) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Prettier.

You can always run Prettier directly from a terminal by typing `yarn format`, or you can install it directly within your IDE by via the following instructions:

Additional specific checks are done by ESLint. Please run `yarn lint-check` or `yarn lint-fix` (the latter if you want to auto-fix all possible issues) and address all raised issues. If any issues seem incompatible or irrelevant to this project, add them to .eslintrc and either demote to warnings or mute entirely.

### [VSCode](https://marketplace.visualstudio.com/items?itemName=esbenp.prettier-vscode)

Launch VS Code Quick Open (Ctrl+P/Cmd+P), paste the following command, and press enter.

```
ext install esbenp.prettier-vscode
```

To ensure that this extension is used over other extensions you may have installed, be sure to set it as the default formatter in your VS Code settings. This setting can be set for all languages or by a specific language.

```
{
  "editor.defaultFormatter": "esbenp.prettier-vscode",
  "[javascript]": {
    "editor.defaultFormatter": "esbenp.prettier-vscode"
  }
}
```
