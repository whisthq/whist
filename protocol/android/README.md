## Fractal Android/Chromebook Client


1: Install Android Studio and choose a path for Android SDK


2: In the small android studio window, on the bottom right, click "configure" and then "SDK manager"


3: In the manager, choose the tab SDK tool and click cmake and ndk


4: Download the ndk 13b from the website: https://developer.android.com/ndk/downloads/older_releases and note the path. It is recommended to place this ndk in the same folder as the latest ndk on your machine.


5: In the project, update the file local.property with the ndk and sdk paths on your machine

sdk.dir=D\:\\Android\\Sdk

ndk.dir=D\:\\Android\\Sdk\\ndk\\13b


6: Navigate to `android/app` and run `keytool -genkey -v -keystore file.jks -alias key -keypass password -storepass password -keyalg RSA -keysize 2048 -validity 10000` and follow the prompts to generate the keystore for build.


7: In the main directory (`protocol`), run `make .` (or whatever the equivalent command is for your system)


8: Open the project folder with android studio and compile. The apk will be released in the folder app/build/outputs/apk/release
