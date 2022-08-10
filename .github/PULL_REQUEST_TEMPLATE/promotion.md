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

- [ ] Cloud tabs successfully connect and play video & audio smoothly
- [ ] All text & image clipboard work in both directions on cloud tabs
- [ ] File upload & download work in both directions on cloud tabs
- [ ] Dragging-and-dropping an image into a cloud tab works correctly
- [ ] All input gestures like scrolling, pinch-to-zoom and keyboard inputs are processed properly on cloud tabs
- [ ] Resizing, minimizing and maximizing a cloud tab works correctly
- [ ] Cloud tabs use minimal CPU and RAM on the system, no matter how many are open concurrently
- [ ] Redirecting Zoom/Spotify/Discord from cloud tabs to desktop/local works correctly

### Backend

- [ ] Users successfully get assigned to a mandelbox in the closest datacenter to them
- [ ] The first cloud tab launches in <10 seconds
- [ ] User's local settings for language, dark mode, DPI, keyboard repeat, etc. are properly set
- [ ] User configs for cloud tabs are successfully stored across Whist sessions

### Chromium Integration

See the [whisthq/brave-core](https://github.com/whisthq/brave-core/) promotion PR for details.

## Merge Instructions

Once the promotion PR is open and well tested, it can be merged from an engineer with force-push access via:

```
git checkout <staging|dev>
git merge <dev|staging> --ff-only
git push -f
```
