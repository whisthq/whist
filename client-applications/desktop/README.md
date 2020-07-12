# Fractal Desktop Applications

![Electron CI](https://github.com/fractalcomputers/client-applications/workflows/Electron%20CI/badge.svg)

This folder contains the code for the Fractal desktop applications running on Windows, MacOS and Linux Ubuntu. The applications are built with cross-platform compatibility using ElectronJS. This repository contains all the directions for building the applications locally and for publishing them for production on each of the following OSes:
-   Windows 10
-   MacOS 10.10+
-   Linux Ubuntu 18.04+

## Install

First, clone the repo via git:

`git clone https://github.com/fractalcomputers/client-applications.git`

And then install the dependencies with yarn:

`cd client-applications/desktop && yarn -i`

## Starting Development

Before starting development, you need to initialize yarn by running `yarn -i`, which will create the `yarn.lock` file and install all of the `node_modules`. If you still experience issues with starting the dev environment, you might need to run `yarn upgrade`, which will upgrade all the dependencies. It's a good idea to do so periodically to keep the application up-to-date. You can automatically clean unnecessary files with `yarn autoclean --init && yarn autoclean --force` as needed.

To start the application in the `dev` environment, run `yarn dev`. To start development with a custom port, run `set PORT={number} && yarn dev`. This will start the Electron application, but will not fetch the Fractal protocol, which means you can only use this to test the application itself, unless you manually cloned and built the protocol yourself. If you're looking to test launching the Fractal protocol from the application, see **Packaging for Production** below. 

This repository has basic continuous integration through GitHub Actions. For every commit or PR to the master branch, it will attempt to build the Electron application on Windows, Mac and Ubuntu, and format the code with Prettier. You should make sure that all tests pass under the Actions tab. If you need help with the Electron CI, check [here](https://github.com/samuelmeuli/action-electron-builder).

## Packaging for Production

In order to properly test the application, you need to package the application and the Fractal protocol together into an installer executable for your local platform. There are two scripts, `build.bat` (Windows) and `build.sh` (MacOS/Linux), which automate the process of packaging locally, for internal publishing, and for publishing to production, by calling `setVersion.ps1` and `setVersion.gyp` to update `package.json`, which is where the bucket and version numbers are stored in Electron.

If you package locally, this will **NOT** publish the application to production, but will only build an identical installer which you can install locally and test from the perspective of a user, before publishing live. It can also publish an internal installer executable which can be distributed within the company for more testing, provided you specify the proper AWS S3 bucket. The installer executable will be in `client-applications/desktop/release` as a `.dmg` (MacOS), `.exe` (Windows) or `.deb` (Linux Ubuntu). You need to package for a platform from that platform; for instance you can only package the Windows application from a Windows computer.

#### MacOS Notarizing

To package the MacOS application, it needs to be notarized. This means it needs to be uploaded to Apple's servers and scanned for viruses and malware. This is all automated as part of Electron, although you need to have the Fractal Apple Developper Certificate in your MacOS Keychain for this to be successful. You can download the certificate from AWS S3 on [this link](https://fractal-private-dev.s3.amazonaws.com/fractal-apple-codesigning-certificate.p12) assuming you have access to the Fractal AWS organization, and then install it by double-clicking the `.p12` certificate file. The application will get notarized seamlessly when running the build script. Once you have the certificate in your Keychain, you can proceed to the testing steps below.

### Local Testing

To package for local testing, you can run the scripts with a subset of the parameters, where branch represents the protocol branch to package, and is likely either `master`, `dev` or `staging`. This will clone the Fractal protocol, build it locally and package it and the application into a `.dmg` for you to install and test.

```
# Windows
build.bat --branch [BRANCH] --publish false

## MacOS/Linux
./build.sh --branch [BRANCH] --publish false
```

### Internal Publishing

To package to an internal, Fractal-only AWS S3 bucket to distribute and test across the company, you can run the same script while incrementing the internal version number and specifying one of the internal testing S3 buckets:
- Windows (internal): `fractal-applications-testing`
- MacOS (internal): `fractal-mac-application-testing`

```
# Windows
build.bat --branch [BRANCH] --version [VERSION] --bucket [BUCKET] --publish false

## MacOS/Linux
./build.sh --branch [BRANCH] --version [VERSION] --bucket [BUCKET] --publish false
```

This will again clone the protocol and package the application, but will also upload its executable to the bucket and auto-update the internal applications within Fractal.

## Publishing to Production

If you're ready to publish an update to production, as part of our [Release Schedule](https://www.notion.so/fractalcomputers/Release-Schedule-Drafting-c29cbe11c5f94cedb9c01aaa6d0d1ca4), then it is time to publish. You can publish by running the same build script, and specifying the AWS S3 bucket which is associated with the platform you are publishing for:
-   Windows: `fractal-applications-release`
-   MacOS: `fractal-mac-application-release`
-   Linux: `fractal-linux-application-release`

First, go to `node_modules/builder-util-runtime/out/httpExecutor.js`, and change the timeout on Line 319 from `60 * 1000` to `60 * 1000 * 1000`. This is necessary to avoid timeout errors for connection in the production application.

Then, publish by running the appropriate script with `--publish` flag set to `true` and incrementing the version number from what is currently in production. You need to increment the version number by `0.0.1`, unless it is a major release, in which case increment by `0.1.0` (e.g. 1.4.6 becomes 1.5.0), from what it currently is. Electron will only auto-update if the version number is higher than what is currently deployed to production. This will also notify the team in Slack.

```
# Windows
build.bat --branch [BRANCH] --version [VERSION] --bucket [BUCKET] --publish true

## MacOS/Linux
./build.sh --branch [BRANCH] --version [VERSION] --bucket [BUCKET] --publish true
```

Finally, git commit and git push to this repository so that the most current production version number is kept track of, even if you only updated the version number.

The production executables are hosted [here](https://s3.console.aws.amazon.com/s3/home?region=us-east-1#).

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io/) before making any PRs. You may find a variety of tutorial online for your personal setup. You can always run Prettier via the command-line by running `yarn format`. 

### [VSCode](https://marketplace.visualstudio.com/items?itemName=esbenp.prettier-vscode)

Launch VS Code Quick Open (Ctrl+P), paste the following command, and press enter.

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
