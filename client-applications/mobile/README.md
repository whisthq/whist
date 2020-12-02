# Fractal Mobile Applications

This repository contains the code for the Fractal mobile applications running on Android/Chromebooks and iOS/iPads. This repository contains all the directions for building the applications locally and for publishing them for production on each of the following OSes:

- Android/ChromeOS
  - x86_64
  - x86
  - arm64-v8a
  - armeabi-v7a
- iOS/iPadOS

## Starting Development

1. We will be using Expo to run locally on Android emulators. To do this, make sure you have `npm` and Node installed, and type `npm install -g expo-cli` in your terminal.

2. In your terminal, `cd` into this folder, and run `expo start.` This will open up your React Native app on localhost.

3. If you happen to have an Android device, connect your Android device to your computer via USB and select the "Start Android emulator" option in the left-hand tab. If you don't, you'll need to download Android Studio: https://developer.android.com/studio.

4. Once Android Studio is downloaded, create a new project and accept all of the default options. Once your new blank project has loaded, navigate to the top right-hand corner and click on the icon for the "AVD Manager."

5. From the AVD Manager, select the Android device you want to emulate. You'll have to follow the setup process to download the operating system of that Android device, which will take some time.

6. After downloading the operating system and finishing the above setup process, you should see an emulated Android device with a black screen appear. Wait about 5-10 minutes for the device to boot up (this will happen automatically).

7. Once the device is booted up, return to the Expo page running on localhost (from step 2) and select the "Start Android emulator" option.

8. You should see this React Native app appear on the screen of the emulated device!

## Packaging and Publishing

TBD

## Continous Integration

TBD

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io/) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Prettier.

You can always run Prettier directly from a terminal by typing `yarn format`, or you can install it directly within your IDE by via the following instructions:

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
