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

The Fractal protocol is linked to this repository as a Git submodule, which you first need to update to the latest commit before using, by running:

```
git submodule update --init --recursive && cd protocol && git checkout [BRANCH] && git pull origin [BRANCH] && cd ..
```

The branch can be any of the existing ones on the protocol repository, but is most likely going to be `dev` if you want to test the upcoming release, or `master` if you want to test the current release. This will ensure you have the latest protocol to test the application. Note that this will fetch the protocol code, and that you will need to compile it in order to use the protocol executable, which can be done via the build scripts (see below) and requires Cmake. In order to make sure that your system has everything needed to compile the protocol, or if you experience issues with compiling the protocol, you should refer to the protocol repository README for instructions.

If you get access denied issues, you need to set-up your local SSH key with your GitHub account, which you can do by following this [tutorial](https://help.github.com/en/github/authenticating-to-github/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent). After running this command, you will have latest the setup-scripts code locally and can call the files as normal.

Before starting development, you need to initialize yarn by running `yarn -i`, which will create the `yarn.lock` file and install all of the `node_modules`. If you still experience issues with starting the dev environment, you might need to run `yarn upgrade`, which will upgrade all the dependencies. It's a good idea to do so periodically to keep the application up-to-date. You can automatically clean unnecessary files with `yarn autoclean --init && yarn autoclean --force` as needed.

To start the application in the `dev` environment, run `yarn dev`. To start development with a custom port, run `set PORT={number} && yarn dev`.

This repository has basic continuous integration through GitHub Actions. For every commit or PR to the master branch, it will attempt to build the Electron application on Windows, Mac and Ubuntu, and format the code with Prettier. You should make sure that all tests pass under the Actions tab. If you need help with the Electron CI, check [here](https://github.com/samuelmeuli/action-electron-builder).

## Packaging for Production

In order to properly test the application, you need to package the application and the Fractal protocol together into an installer executable for your local platform. This will **NOT** publish the application to production, but will only build an indentical installer which you can install locally and test from the perspective of a user before publishing. The installer executable will be in `client-applications/desktop/release`.

### MacOS/Linux




//tmp






[https://fractal-private-dev.s3.amazonaws.com/fractal-apple-codesigning-certificate.p12](https://fractal-private-dev.s3.amazonaws.com/fractal-apple-codesigning-certificate.p12)





Run `./build.sh` in a terminal. This will delete any prior Fractal protocol folder, pull the recent master branch, and package it locally. You must also install Cmake; refer to the Fractal protocol repository for installation instructions.
`build.sh` now has cli arguments. By default `build.sh` will create a release and sign it. To build a fractal client without signing it and running it in dev, use `DEV=yes ./build.sh`. It also starts the dev client after building.
To build a release of the client, sign it, and upload it, use `RELEASE=yes ./build.sh` ensure that you follow the "Publishing to Production" instructions beforehand.

### Windows

Run `build.bat` in an x86_x64 Visual Studio Developer Command Prompt. This will delete any prior Fractal protocol folder, pull the recent master branch, and package it locally. You MUST use this specific command prompt to compile the protocol; if you do not the Electron application will package anyway, but the protocol will not packaged with it. If you do not have this command prompt, you need to install [Microsoft Visual Studio Community 2019](https://visualstudio.microsoft.com/downloads/) and select `Desktop Development with C++` add-on in the installer. You must also install [Cmake](https://cmake.org/download/). For more information on compiling on Windows, refer to the Fractal protocol repository.

## Publishing to Production

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
