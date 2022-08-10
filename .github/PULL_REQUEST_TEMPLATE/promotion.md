## Whist Browser Promotion

_Associated [whisthq/brave-core](https://github.com/whisthq/brave-core) promotion PR:_ #

This is the Whist monorepo side of the promotion for the Whist Browser. The associated Brave/Chromium promotion should be open before this PR is merged and linked above. The deployment gets triggered automatically from merging this promotion PR, and so the Brave/Chromium PR needs to be tested and merged first.

## Promotion Testing

### Nightly (dev)

- [macOS (x64)](https://s3.amazonaws.com/whist-browser-macos-x64-dev/whist-browser-macos-x64-dev.tar.gz)
- [macOS (arm64)](https://s3.amazonaws.com/whist-browser-macos-arm64-dev/whist-browser-macos-arm64-dev.tar.gz)

You can download the Nightly build for [macOS (x64)]() and [macOS (arm64)]() here.

### Beta (staging)

- [macOS (x64)](https://s3.amazonaws.com/whist-browser-macos-x64-staging/whist-browser-macos-x64-staging.tar.gz)
- [macOS (arm64)](https://s3.amazonaws.com/whist-browser-macos-arm64-staging/whist-browser-macos-arm64-staging.tar.gz)

## Promotion Checklist

### General

- [ ] The associated PR in [whisthq/brave-core](https://github.com/whisthq/brave-core) is open, tested, and merged
- [ ] This promotion is backwards-compatible
- [ ] The [Whist Release Log](https://www.notion.so/whisthq/Whist-Release-Log-c7ea1639eb734d90bd48c34924d72f0a) is up-to-date (`prod` release only)

### Protocol

- [ ] Cloud tabs successfully connect and play video & audio
- [ ] All text & image clipboard work in both directions on cloud tabs
- [ ] File upload & download work in both directions on cloud tabs





- [ ] Drag-and-drop an image from local to Whist


- [ ] Do 30 seconds of TypeRacer without no repeated characters, lag, etc.
- [ ] Watch a 1 minute YouTube video with no video or audio stutters
- [ ] Whist successfully receives smooth scrolling and pinch-to-zoom input (i.e. on Figma)
- [ ] CPU usage is low (below 30%)
- [ ] RAM usage is low (below 150 MB)
- [ ] Redirecting Zoom/Spotify/Discord from Whist to local is seamless

- [ ] Rapid resizing of a cloud tab works seamlessly
- [ ] Minimizing and maximizing a cloud works without interruptions


- [ ] All browser functionalities (scroll, pinch, zoom, file drag/upload/download, etc.) work as expected





### Backend


- [ ] Going to Google Maps correctly shows the user's approximate location
- [ ] User's local settings for language, dark mode, DPI, keyboard repeat, etc. are properly set


- [ ] Startup time for Whist is <10 seconds
- [ ] Typing "where am I" in Google shows that I'm connected to the closest datacenter
- [ ] User sessions are successfully stored across Whist sessions











### Chromium Integration

See the [whisthq/brave-core](https://github.com/whisthq/brave-core/) promotion PR for details.

## Merge Instructions

Once the promotion PR is open and well tested, it can be merged from an engineer with force-push access via:

```
git checkout <staging|dev>
git merge <dev|staging> --ff-only
git push -f
```
