# Fractal Desktop Applications

![Electron CI](https://github.com/fractalcomputers/client-applications/workflows/Electron%20CI/badge.svg)

This folder contains the code for the Fractal desktop applications running on Windows, MacOS and Linux Ubuntu. The applications are built with cross-platform compatibility using ElectronJS. This repository contains all the directions for building the applications locally and for publishing them for production on each of the following OSes:
-   Windows
-   MacOS
-   Linux Ubuntu

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

In order to properly test the application, you need to package the application and the Fractal protocol together into an installer executable for your local platform. This will **NOT** publish the application to production, but will only build an identical installer which you can install locally and test from the perspective of a user, before publishing live. The installer executable will be in `client-applications/desktop/release` as a `.dmg` (MacOS), `.exe` (Windows) or `.deb` (Linux Ubuntu). You need to package for a platform from that platform, for instance you can only package the Windows application from a Windows computer.

The `build.sh` and `build.bat` scripts automate the process of packaging the Fractal protocol and the application together. In order to use them, you **need** to have all the tools needed to build the protocol on your local machine. If you've never compiled the protocol before, or are having issues, refer to the protocol repository [here](https://github.com/fractalcomputers/protocol).

### MacOS/Linux

To package for local testing on Unix systems, simply run `build.sh` with the appropriate protocol branch you'd like to test, likely `dev` or `master`.

```
./build.sh --branch [BRANCH]
```

This will clone the Fractal protocol, build it locally and package it and the application into a `.dmg` for you to install and test. 

#### MacOS Notarizing

To package the MacOS application, it needs to be notarized, which means it needs to be uploaded to Apple's servers and scanned for viruses and malware. This is all automated as part of Electron, although you need to have the Fractal Apple Developper Certificate in your MacOS Keychain for this to be successful. You can download the certificate from AWS S3 on [this link](https://fractal-private-dev.s3.amazonaws.com/fractal-apple-codesigning-certificate.p12) assuming you have access to the Fractal AWS organization, and then install it by double-clicking the `.p12` certificate file. The application will get notarized seamlessly when running the build script.

### Windows

To package for local testing on Windows systems, simply run `build.bat` from an x86_x64 Visual Studio Developer Command Prompt with the appropriate protocol branch you'd like to test, likely `dev` or `master`. You MUST use this specific command prompt to compile the protocol; if you do not the Electron application will package anyway, but the protocol will not packaged with it. If you need help setting up Visual Studio, refer to the protocol repository for instructions on how to compile.

```
build.bat --branch [BRANCH]
```

This will clone the Fractal protocol, build it locally and package it and the application into a `.exe` for you to install and test. 

## Publishing to Production

If you have tested the packaged application locally and are ready to update the client applications, including the client protocol, as part of our [Release Schedule](https://www.notion.so/fractalcomputers/Release-Schedule-Drafting-c29cbe11c5f94cedb9c01aaa6d0d1ca4), then it is time to publish. You can publish by running the same build script, and specifying the AWS S3 bucket which is associated with the platform you are publishing for. The buckets are:

-   Windows: `fractal-applications-release`
-   Windows (internal): `fractal-applications-testing`
-   MacOS: `fractal-mac-application-release`
-   MacOS (internal): `fractal-mac-application-testing`
-   Linux: `fractal-linux-application-release`



```
./build.sh --branch [BRANCH]
```


Before publishing for production, make sure to package for production (see above) and test locally. In order to publish to production, you will run all the steps for packaging and need to make sure all the Cmake and other dependencies listed above are satisfied on your system. Once you are ready to publish for auto-update to the Fractal users, you need to do a few things:

1- Go into `/desktop/package.json` and update the `"bucket":` field to the proper bucket for the operating system you are publishing for:

-   Windows: `fractal-applications-release`
-   MacOS: `fractal-mac-application-release`
-   Linux: `fractal-linux-application-release`

2- Go to `node_modules/builder-util-runtime/out/httpExecutor.js`, and change the timeout on Line 319 from `60 * 1000` to `60 * 1000 * 1000`. This is necessary to avoid timeout errors for connection in the production application.

3- Increment the version number in `desktop/app/package.json` by `0.0.1`, unless it is a major release, in which case increment by `0.1.0` (e.g.: 1.4.6 becomes 1.5.0).

4- Then, run `RELEASE=yes ./build.sh` (MacOS/Linux) or `publish.bat` (Windows - in an x86_64 Visual Studio Developer Command Prompt) to publish for the respective OS. This will fetch the latest Fractal Protocol, set proper file permissions, set the executable icon, upgrade yarn and run `yarn package-ci` to publish to the S3 bucket.

5- Lastly, git commit and git push to this repository so that the most current production version number is kept track of, even if you only updated the version number.


//tmp






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
