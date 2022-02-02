**High-Level Description of Features**

**Promotion Testing**

In order for this PR to be merged, all of the following items need to be tested and checked. Make sure to perform these tests from the correct packaged app (on the `dev` > `staging` promotion, use the `dev` app, and on the `staging` > `prod` promotion, use the `staging` app). Everyone is encouraged to do these tests, the more people that test the better! If you don't have them already installed, you can download the applications from the following links:

- [macOS X86_64 Application (Dev)](https://fractal-chromium-macos-dev.s3.amazonaws.com/Whist.dmg)
- [macOS X86_64 Application (Staging)](https://fractal-chromium-macos-staging.s3.amazonaws.com/Whist.dmg)
- [macOS ARM64 Application (Dev)](https://fractal-chromium-macos-arm64-dev.s3.amazonaws.com/Whist.dmg)
- [macOS ARM64 Application (Staging)](https://fractal-chromium-macos-arm64-staging.s3.amazonaws.com/Whist.dmg)

**Checklist**

- [ ] This promotion is backwards compatible
- [ ] Whist successfully auto-updates
- [ ] Whist successfully launches
- [ ] Whist successfully plays audio
- [ ] Typing "where am I" in Google shows that I'm connected to the closest datacenter
- [ ] Copy text from Whist to local clipboard
- [ ] Copy text from local clipboard to Whist
- [ ] Copy image from Whist to local clipboard
- [ ] Copy image from local clipboard to Whist
- [ ] Resizing SDL window to minimum resolution works
- [ ] Rapid resizing of SDL window works
- [ ] Minimizing and maximizing SDL window works without interruptions
- [ ] CPU usage is low (macOS)
- [ ] Do 30 seconds of TypeRacer, no repeated characters, lag, etc.
- [ ] Watch a 1 minute YouTube video with no video or audio stutters
- [ ] Go to Figma and verify pinch-to-zoom works
- [ ] Verify that smooth two-finger scrolling works
- [ ] Log into any website, close Whist, and re-open. I am still logged in.
- [ ] Sign out button on omnibar works
- [ ] Session logs successfully uploaded to Amplitude
- [ ] Website successfully deployed to Netlify
