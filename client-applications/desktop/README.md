# Fractal Desktop Applications

This folder contains the code for the Fractal desktop applications running on Windows, MacOS and Linux Ubuntu. The applications are built cross-platform using ElectronJS.

Currently supported:
- Windows
- MacOS
- Linux

## Note
We currently have to manually fix a node-modules error by modifying 'node_modules/builder-util-runtime/out/httpExecutor.js' and changing the setTimeout() in Line 322. This is the best fix for an update timeout error that I was able to find.

## Install

First, clone the repo via git:

```git clone https://github.com/fractalcomputers/client-applications.git```

And then install the dependencies with yarn.

```cd client-applications```

```yarn```

## Starting Development

Start the app in the `dev` environment. This starts the renderer process in [**hot-module-replacement**](https://webpack.js.org/guides/hmr-react/) mode and starts a webpack dev server that sends hot updates to the renderer process (note that this will only build the application, it won't fetch the latest Fractal protocol):

```yarn dev```

Before doing this, you will need to run ```yarn -i``` and might need to run ```yarn upgrade``` if you haven't upgraded yarn in a while. You can automatically clean unnecessary files with ```yarn init --force && yarn autoclean --force``` as needed.

## Packaging for Production

To package apps for the local platform, including fetching and compiling the latest Fractal protocol:

Run ```./build.sh``` (for MacOS/Linux), or ```build.bat``` (for Windows). If you have already downloaded and compiled the latest Fractal protocol, you can simply run ```yarn package```.

This will enable you to get an executable that you can install to test your code locally. The installer executables will be in ```desktop/release```.

## Publishing to Production

Once you are ready to publish for auto-update to the Fractal users, you need to do a few things:

1- Go into ```/desktop/package.json``` and update the ```"bucket": ``` field to the proper bucket for the operating system you are publishing for:
  - Windows: ```fractal-applications-release```
  - MacOS: ```fractal-mac-application-release```
  - Linux: ```fractal-linux-application-release```
  
 2- Increment the version number in ```desktop/app/package.json``` by ```0.0.1```, unless it is a major release, in which case increment by ```0.1.0``` (e.g.: 1.4.6 becomes 1.5.0).

 3- Then, run ```./publish.sh``` (MacOS/Linux) or ```publish.bat``` (Windows) to publish for the respective OS. This will fetch the latest Fractal Protocol, upgrade yarn and run ```yarn package-ci``` to publish to the S3 bucket. 
 
 4- Lastly, git commit and git push to this repository so that the most current production version number is kept track of, even if you only updated the version number.

The production executables are hosted at: https://s3.console.aws.amazon.com/s3/home?region=us-east-1#
