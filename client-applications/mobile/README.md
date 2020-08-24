# Fractal Mobile Applications

This repository contains the code for the Fractal mobile applications, which run on Android, including Chromebooks, and iOS, including iPads. The application will be developped in React Native to ensure cross-platform compatibility.

## Running locally

1. We will be using Expo to run locally on Android emulators. To do this, make sure you have `npm` and Node installed, and type `npm install -g expo-cli` in your terminal.

2. In your terminal, `cd` into this folder, and run `expo start.` This will open up your React Native app on localhost.

3. If you happen to have an Android device, connect your Android device to your computer via USB and select the "Start Android emulator" option in the left-hand tab. If you don't, you'll need to download Android Studio: https://developer.android.com/studio.

4. Once Android Studio is downloaded, create a new project and accept all of the default options. Once your new blank project has loaded, navigate to the top right-hand corner and click on the icon for the "AVD Manager."

5. From the AVD Manager, select the Android device you want to emulate. You'll have to follow the setup process to download the operating system of that Android device, which will take some time. 

6. After downloading the operating system and finishing the above setup process, you should see an emulated Android device with a black screen appear. Wait about 5-10 minutes for the device to boot up (this will happen automatically).

7. Once the device is booted up, return to the Expo page running on localhost (from step 2) and select the "Start Android emulator" option.

8. You should see this React Native app appear on the screen of the emulated device!


