# Fractal Desktop Applications

This folder contains the code for the Fractal desktop applications running on Windows and MacOS (Linux currently not supported). The applications are built with cross-platform compatibility using ElectronJS.

## Setting Up for Development

1. Make sure you have `yarn` installed on your computer. You can install it [here](https://classic.yarnpkg.com/en/docs/install/#mac-stable).

2. `cd` into the `desktop` folder.

3. Run `yarn` to install all dependencies on your local machine. If dependency issues occur, try running `rm -rf node_modules/` and then `yarn` again.

4. To start the application in the `dev` environment, run `yarn dev`. To start development with a custom port, run `yarn cross-env PORT={number} yarn dev`. Note: `yarn dev` will start the Electron application, but will not fetch the Fractal protocol, which is necessary to stream apps. If you're looking to test launching the Fractal protocol from the application, see **Packaging for Production** below.

## How To Contribute

Before making a pull request, ensure that the following steps are taken:

1. Make sure the application is fully working both in `dev` (via `yarn dev`) and packaged (see **Packaging for Production**). This is important because the Electron app can behave differently when packaged. For example, file paths will change.

2. Make sure that your code follows the guidelines outlined in our [React coding philsophy](https://www.notion.so/tryfractal/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289).

3. Lint your code by running `yarn lint-fix`. If this does not pass, your code will fail Github CI.

4. Run all test files by running `yarn test`. If this does not pass, your code will fail Github CI. NOTE: If you have written new functions, make sure it has a corresponding test, or code reviewers will request changes on your PR.

5. Rebase against `dev` by pulling `dev` and running `git rebase dev`.

Finally, you can open PR to `dev`.

### How To Clear The App's Cache

Some pieces of state, such as the user's authentication token, are stored in the file system and persist between app launches. You can clear these files to re-trigger behavior, such as a re-running the login flow, by deleting/modifying/reviewing the files at:

-   On macOS, look in `~/Library/Application\ Support/{Electron,Fractal}/config.json`
-   On Windows, look in `C:\Users\<user>\AppData\Roaming\{Electron,Fractal}\Cache\config.json`

The unpackaged app will have `Electron` in the path while the packaged app will have `Fractal`.

## Testing the client app with the protocol

The client app launches the protocol, but the protocol needs to be built first.

1. Go to the `/protocol/` README to install all the necessary prerequisites.

2. In the `client-applications/desktop` folder, run `publish --help` on Windows in an x86-x64 terminal (comes with Visual Studio) or `./publish.sh --help` on Mac for instructions on how to run the `publish` script. Once the script is run, the installer executable will be in `client-applications/desktop/release`. No cross-compilation is possible, e.g. you can only package the Windows application from a Windows computer.

#### MacOS Notarizing

Before you can package the MacOS application it needs to be notarized. This means that it needs to be uploaded to Apple's servers and scanned for viruses and malware. This is all automated as part of Electron, although you need to have the Fractal Apple Developer Certificate in your MacOS Keychain for this work successfully. You can download the certificate from AWS S3 on [this link](https://fractal-dev-secrets.s3.amazonaws.com/fractal-apple-codesigning-certificate.p12) assuming you have access to the Fractal AWS organization, and then install it by double-clicking the `.p12` certificate file. The application will get notarized as part of the regular build script.

In order for this to work, you need to have installed the latest version of Xcode (which you can install from the macOS App Store), and have opened it _at least_ once following installation, which will prompt you to install additional components. If your macOS version is too old to install Xcode directly from the macOS App Store, you can manually download the macOS SDK (which you'd normally obtain through Xcode) for your macOS version and place it in the right subfolder. Here's an example for macOS 10.14:

```
# Explicitly retrieve macOS 10.14 SDK
wget https://github.com/phracker/MacOSX-SDKs/releases/download/10.15/MacOSX10.14.sdk.tar.xz

# Untar it
xz -d MacOSX10.14.sdk.tar.xz
tar -xf MacOSX10.14.sdk.tar

# Move it to the right folder for building the protocol
mv MacOSX10.14.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
```

Once those components are installed, you need to open up a terminal and run `xcode-select --install` to install the Xcode CLI, which is necessary for the notarizing to work. Once all of these are done, you should have no problem with the notarization process as part of the packaging of the application.

### Publishing New Versions

Fractal runs two update channels, `production` and `testing`. The `dev` branch should be published automatically to `testing`, while `production` should match `master`. The build script has a special `noupdates` channel which should be used for any builds that aren't on one of these branches.

Any CI generated builds are also stored in GitHub Releases which can be manually downloaded and used.

#### Update Channels

There is a channel for `testing` and `production` on each platform. These channels are backed by AWS S3 buckets ([here](https://s3.console.aws.amazon.com/s3/home?region=us-east-1#)) that follow a file structure and metadata schema specified by [electron-builder's publish system](https://www.electron.build/configuration/publish) (it's basically the executable installer + a well-known YAML file with metadata like file hash, file name, and release date which is used for knowing when an update is available).

## Continous Integration

This repository has basic continuous integration through GitHub Actions. For every PR to `dev`, `staging`, or `master`, GitHub Actions will attempt to build the bundled application on Windows-64bit, macOS-64bit, and Linux-64bit. It will upload these builds to the GitHub Releases tab with a version identifier corresponding to the current git ref (eg. branch) and the current date.

New builds from the `dev` will also be pushed out on the `testing` channel (not yet done).

Moreover, new builds from [/protocol](https://github.com/fractal/fractal/tree/dev/protocol) will trigger builds in this repository on their corresponding branch (or on `dev` if there is no appropriate corresponding branch). Similarly, new protocol builds on `dev` will also trigger a new build to be sent out on the `testing` channel.

Additionally, [style](#Styling) checks will be run to verify that you formatted your code via Prettier. You should make sure that all tests pass under the Actions tab.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io/) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Prettier.

You can always run Prettier directly from a terminal by typing `yarn format`, or you can install it directly within your IDE by via the following instructions:

### [VSCode](https://marketplace.visualstudio.com/items?itemName=esbenp.prettier-vscode)

Additional specific checks are done by ESLint. Please run `yarn lint-check` or `yarn lint-fix` (the latter if you want to auto-fix all possible issues) and address all raised issues. If any issues seem incompatible or irrelevant to this project, add them to .eslintrc and either demote to warnings or mute entirely.

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
