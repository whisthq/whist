## Fractal Android/Chromebook Client


1: Install Android Studio and choose a path for Android SDK


2: In the small android studio window, on the bottom right, click "configure" and then "SDK manager"


3: In the manager, choose the tab SDK tool and click cmake and ndk


4: Download the ndk 13b from the website: https://developer.android.com/ndk/downloads/older_releases and note the path. It is recommended to place this ndk in the same folder as the latest ndk on your machine. (We are actually using the latest NDK instead of this legacy NDK, but if there are issues, it may be worth rolling back to an older NDK version).


5: In the project, update the file local.property with the ndk and sdk paths on your machine

sdk.dir=D\:\\Android\\Sdk

ndk.dir=D\:\\Android\\Sdk\\ndk\\13b


6: Navigate to `android/app` and run `keytool -genkey -v -keystore file.jks -alias key -keypass password -storepass password -keyalg RSA -keysize 2048 -validity 10000` and follow the prompts to generate the keystore for build.


7: In the main directory (`protocol`), run `make .` (or whatever the equivalent command is for your system)


8: THIS STEP SHOULD EVENTUALLY BE REMOVED: change the server IP address in `app/src/main/java/org/libsdl/app/SDLActivity.java` in the `getArguments` method.


9: In Android Studio, click File > Sync Project with Gradle Files


10: Open the project folder with android studio and compile. The apk will be released in the folder app/build/outputs/apk/ under either the release or debug folder


11: If testing on an emulator, make sure to select an x86_64 image for testing. The emulator doesn't run very well, though, so this is not recommended. Instead, either send the file to yourself and install on a Chromebook, or connect your Android device to your development machine and click the "run" button to install and run on your device. By default this will happen in debug mode, so to test the release apk, go to Build > Generate Signed Bundle / APK and follow the instructions. Then go to Build > Select Build Variant to select release mode as the one to be run on your device.

**TODO**:
When linking up to the client application, a few things need to be changed to allow user-defined arguments in places where they are currently hard-coded:
* `app/src/main/java/org/libsdl/app/SDLActivity.java`: `getArguments` method - this method currently returns a set of hard-coded arguments (IP address of host and width/height of display)
* `app/src/main/java/org/libsdl/app/SDLActivity.java`: `SDLMain` class - the `run` method calls `SDLActivity.setWindowStyle(true);` which sets the application to default to fullscreen mode. If the client selects windowed mode, you must pass `false` into this method call.
