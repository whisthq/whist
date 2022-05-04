## High-Level Description of Features

## Promotion Testing

All the below items need to be tested and checked for this promotion to be merged. Make sure to perform these checks on the correct Whist application. If you don't have them already installed, you can download the applications from the following links:

### `dev`

- [macOS (X86_64)](https://whist-electron-macos-x64-dev.s3.amazonaws.com/Whist.dmg)
- [macOS (ARM64)](https://whist-electron-macos-arm64-dev.s3.amazonaws.com/Whist.dmg)
- [Windows](https://whist-electron-windows-dev.s3.amazonaws.com/Whist.exe)

### `staging`

- [macOS (X86_64)](https://whist-electron-macos-x64-staging.s3.amazonaws.com/Whist.dmg)
- [macOS (ARM64)](https://whist-electron-macos-arm64-staging.s3.amazonaws.com/Whist.dmg)
- [Windows](https://whist-electron-windows-staging.s3.amazonaws.com/Whist.exe)

### Checklist

#### General

- [ ] This promotion is backwards compatible
- [ ] Website preview successfully deploys to Netlify
- [ ] Whist successfully auto-updates
- [ ] Whist successfully launches
- [ ] Whist successfully plays audio

#### Frontend

- [ ] Sign in to Whist works
- [ ] Importing browser settins from Brave & Chrome works
- [ ] Going to Google Maps correctly shows the user's approximate location
- [ ] User local settings for language, dark mode, DPI, keyboard repeat, etc. are properly set
- [ ] Accessing and updating the Stripe subscription from the omnibar works
- [ ] Live chat support with Intercom from the omnibar works
- [ ] Sign out button on omnibar works

#### Backend

- [ ] Startup time for Whist is <15 seconds
- [ ] Typing "where am I" in Google shows that I'm connected to the closest datacenter
- [ ] User sessions are successfully stored across Whist sessions

#### Streaming

- [ ] Do 30 seconds of TypeRacer without no repeated characters, lag, etc.
- [ ] Watch a 1 minute YouTube video with no video or audio stutters
- [ ] Whist successfully receives smooth scrolling and pinch-to-zoom input (i.e. on Figma)
- [ ] CPU usage is low (below 30%)
- [ ] RAM usage is low (below 150 MB)
- [ ] Redirecting Zoom/Spotify/Discord from Whist to local is seamless

#### Display

- [ ] Resizing the SDL window blocks at minimum resolution
- [ ] Rapid resizing of the SDL window works seamlessly
- [ ] Minimizing and maximizing the SDL window works without interruptions

#### Clipboard

- [ ] Copy text from Whist to local clipboard
- [ ] Copy text from local clipboard to Whist
- [ ] Copy image from Whist to local clipboard
- [ ] Copy image from local clipboard to Whist
- [ ] Upload a file from local to Whist
- [ ] Download a file from Whist to local
- [ ] Drag-and-drop an image from local to Whist

#### Analytics

- [ ] Session logs successfully upload to Amplitude
- [ ] Client and server logs successfully upload to Logz.io
