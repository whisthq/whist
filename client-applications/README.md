# Fractal Client Applications

This repository contains the Fractal client applications, which users download to their local device to connect stream applications via Fractal to their local device. Each subfolder contains the appropriate code for its platform, each with a proper README with information on how to develop or build each application.

See [desktop/README.md](desktop/README.md) for details on contributing to the desktop applications project.

## Supported Applications

The following platforms are supported by the Fractal applications (links are to S3 buckets containing the platform's latest `production` release):

- [Windows 10](https://s3.console.aws.amazon.com/s3/buckets/fractal-chromium-windows-prod/?region=us-east-1)
- [MacOS 10.15+](https://s3.console.aws.amazon.com/s3/buckets/fractal-chromium-macos-prod/?region=us-east-1)

For the desktop OS applications listed above, the version requirements come from the Fractal protocol; the Electron-based application should run on older versions as well, if needed for testing.

The following platforms have yet to be supported:

- Linux Ubuntu 18.04+
- iOS/iPadOS
- Android/Chromebook
- Web (the web client will be hosted on `fractal/website`)
