## Fractal Android/Chromebook Client


1: Install Android Studio and chose a path for Android SDK


2: In the small android studio window, on the bottom right, click "configure" and then "SDK manager"


3: In the manager, chose the tab SDK tool and click cmake and ndk


4: Download the ndk 13b from the website: https://developer.android.com/ndk/downloads/older_releases


5: Put it in the same folder than the previous ndk


6: Download SDL sources. Copy the folder android project, it will be the project folder.


7: Compile ffmpeg sources: https://github.com/tanersener/mobile-ffmpeg


8: Dowload or compile OpenSSL sources: https://github.com/leenjewel/openssl_for_ios_and_android


9: Put the files in the corresponding folders ./libs/[ARCH]/


10: In the project, update the file local.property with the ndk path

sdk.dir=D\:\\Android\\Sdk

ndk.dir=D\:\\Android\\Sdk\\ndk\\13b


11: Ope the project folder with android studio and compile. The apk will be released in the folder app/build/apk/release
