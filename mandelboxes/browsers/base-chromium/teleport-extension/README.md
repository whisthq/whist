# Whist Teleport Extension

This project contains the server-side Whist Chromium extension, known as Whist Teleport. It is a browser extension that we install on all server-side browsers (Chrome, Brave) we run, to allow Whist to correlate the state of the server-side browser with the state of the client-side browser (for instance, for file upload and tab switching, mouse pointer locking, but also for some minor QoL improvements to the server-side Chromium experience, like a welcome page).

## Development

This project is developed as a regular TypeScript project with Webpack as the module bundler. Please adhere to the usual Whist code standards for TypeScript.

## Publishing

This project gets installed directly in the Chrome Dockerfile, which gets shipped as part of our standard deployment pipeline. All changes you merge into this project will automatically be bunlded in the next deployment.
