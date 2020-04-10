# Fractal Desktop Applications

This folder contains the code for the Fractal desktop applications running on Windows, MacOS and Linux Ubuntu. The applications are built cross-platform using ElectronJS.

Currently supported:
- Windows
- MacOS
- TODO: Add Linux Support

## Install

First, clone the repo via git:

```git clone https://github.com/fractalcomputers/client-applications.git```

And then install the dependencies with yarn.

```cd client-applications```

```yarn```

## Starting Development

Start the app in the `dev` environment. This starts the renderer process in [**hot-module-replacement**](https://webpack.js.org/guides/hmr-react/) mode and starts a webpack dev server that sends hot updates to the renderer process:

```yarn dev```

Before doing this, you will need to run ```yarn -i``` and might need to run ```yarn upgrade``` if you haven't upgrade your yarn in a while.

## Packaging for Production

To package apps for the local platform:

```yarn package```

This will enable you to get an executable that you can install to test your code locally. The installer executables will be in ```desktop/release```.

## Publishing to Production

Once you are ready to publish for auto-update to the Fractal users, you need to do a few things:
1- Go into ```/desktop/package.json``` and update ```"buchet": ``` to the proper bucket for the operating system you are publishing for:
  - Windows: ```fractal-applications-release```
  - MacOS: ```fractal-mac-application-release```
  - Linux: ```fractal-linux-application-release```
 2- Increment the version number in ```desktop/app/package.json``` by ```0.0.1```, unless it is a major release, in which case increment by ```0.1.0```
 3- Then, run ```./build.sh``` (MacOS) or ```build.bat``` (Windows) to publish. This will fetch the latest Fractal Protocol, upgrade yarn and run ```yarn package-ci``` to publish to the S3 bucket. 

The production executables are hosted at: https://s3.console.aws.amazon.com/s3/buckets/fractal-applications-release/?region=us-east-1
