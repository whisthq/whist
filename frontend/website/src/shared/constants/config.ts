/* eslint-disable */
import React from "react"
import type { WhistEnvironment, WhistConfig } from "@app/shared/types/config"

const environment: WhistEnvironment = {
  dev: {
    sentry_env: "dev",
    client_download_urls: {
      macOS_x64:
        "https://fractal-chromium-macos-dev.s3.amazonaws.com/Whist.dmg",
      macOS_arm64:
        "https://fractal-chromium-macos-arm64-dev.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://whist-electron-windows-dev.s3.amazonaws.com/Whist.exe",
    },
  },
  staging: {
    sentry_env: "staging",
    client_download_urls: {
      macOS_x64:
        "https://fractal-chromium-macos-staging.s3.amazonaws.com/Whist.dmg",
      macOS_arm64:
        "https://fractal-chromium-macos-arm64-staging.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://whist-electron-windows-staging.s3.amazonaws.com/Whist.exe",
    },
  },
  prod: {
    sentry_env: "prod",
    client_download_urls: {
      macOS_x64:
        "https://fractal-chromium-macos-prod.s3.amazonaws.com/Whist.dmg",
      macOS_arm64:
        "https://fractal-chromium-macos-arm64-prod.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://whist-electron-windows-base.s3.amazonaws.com/Whist.exe",
    },
  },
}

export const config: WhistConfig = (() => {
  if (import.meta.env.MODE !== "development")
    return environment[
      import.meta.env.WHIST_ENVIRONMENT as keyof WhistEnvironment
    ]

  return environment.dev
})()
