# Fractal Desktop Applications

This folder contains the code for the Fractal desktop applications running on Windows, MacOS and Linux Ubuntu. The applications are built cross-platform using ElectronJS. This repository contains all the directions for building the applications locally and for publishing them for production.

Currently supported:

-   Windows
-   MacOS
-   Linux Ubuntu

## Install

First, clone the repo via git:

`git clone https://github.com/fractalcomputers/client-applications.git`

And then install the dependencies with yarn.

`cd client-applications/desktop`

`yarn`

## Starting Development

Start the app in the `dev` environment. This starts the renderer process in [**hot-module-replacement**](https://webpack.js.org/guides/hmr-react/) mode and starts a webpack dev server that sends hot updates to the renderer process (note that this will only build the application, it won't fetch the latest Fractal protocol, so you won't be able to run the protocol by simply doing `yarn dev`):

`yarn dev`

Before doing this, you will need to run `yarn -i` and might need to run `yarn upgrade` if you haven't upgraded yarn in a while. You can automatically clean unnecessary files with `yarn autoclean --init && yarn autoclean --force` as needed.

If you would like to fully test the application, including the launch of the Fractal protocol, you need to run part of the build scripts, listed in the next section, to clone and make the protocol for your platform. Compiling the protocol requires Cmake. In order to make sure that your system has everything needed to compile the protocol, you should refer to the protocol repository.

## Packaging for Production

To package apps for the local platform, including fetching and compiling the latest Fractal protocol:

Run `./build.sh` (for MacOS/Linux), or `build.bat` (for Windows). If you have already downloaded and compiled the latest Fractal protocol, you can simply run `yarn package`.

This will enable you to get an executable that you can install to test your code locally. The installer executables will be in `desktop/release`.

## Publishing to Production

Once you are ready to publish for auto-update to the Fractal users, you need to do a few things:

1- Go into `/desktop/package.json` and update the `"bucket":` field to the proper bucket for the operating system you are publishing for:

-   Windows: `fractal-applications-release`
-   MacOS: `fractal-mac-application-release`
-   Linux: `fractal-linux-application-release`

2- Go to `node_modules/builder-util-runtime/out/httpExecutor.js`, and change the timeout on Line 319 from `60 * 1000` to `60 * 1000 * 1000`. This is necessary to avoid timeout errors for connection in the production application.

3- Increment the version number in `desktop/app/package.json` by `0.0.1`, unless it is a major release, in which case increment by `0.1.0` (e.g.: 1.4.6 becomes 1.5.0).

4- Then, run `./publish.sh` (MacOS/Linux) or `publish.bat` (Windows) to publish for the respective OS. This will fetch the latest Fractal Protocol, set proper file permissions, set the executable icon, upgrade yarn and run `yarn package-ci` to publish to the S3 bucket.

5- Lastly, git commit and git push to this repository so that the most current production version number is kept track of, even if you only updated the version number.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io/) before making any PRs. You may find a variety of tutorial online for your personal setup. This README covers how to set it up on VSCode and Sublime.

### Sublime

https://packagecontrol.io/packages/JsPrettier

Install prettier if you haven't yet.

```
# yarn (global):
yarn add prettier
```

The easiest and recommended way to install Js​Prettier is using Package Control. From the application menu, navigate to:  
`Tools` -> `Command Palette...` -> `Package Control: Install Package`, type the word JsPrettier, then select it to complete the installation.

Usage

1. Command Palette: From the command palette (ctrl/cmd + shift + p), type JsPrettier Format Code.
2. Context Menu: Right-click anywhere in the file to bring up the context menu and select JsPrettier Format Code.
3. Key Binding: There is no default key binding to run Prettier, but you can add your own.

We recommend setting `auto_format` to `true` in Sublime so you won't need to worry about the usage methods.

### VSCode

https://marketplace.visualstudio.com/items?itemName=esbenp.prettier-vscode

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
