/* eslint-disable */
import React from "react"
import type { WhistEnvironment, WhistConfig } from "@app/shared/types/config"

const environment: WhistEnvironment = {
  dev: {
    keys: {
      NOTION_API_KEY: "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ",
    },
    sentry_env: "dev",
    client_download_urls: {
      macOS_x64:
        "https://fractal-chromium-macos-dev.s3.amazonaws.com/Whist.dmg",
      macOS_arm64:
        "https://fractal-chromium-macos-arm64-dev.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://fractal-chromium-windows-dev.s3.amazonaws.com/Whist.exe",
    },
  },
  staging: {
    keys: {
      NOTION_API_KEY: "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ",
    },
    sentry_env: "staging",
    client_download_urls: {
      macOS_x64:
        "https://fractal-chromium-macos-staging.s3.amazonaws.com/Whist.dmg",
      macOS_arm64:
        "https://fractal-chromium-macos-arm64-staging.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://fractal-chromium-windows-staging.s3.amazonaws.com/Whist.exe",
    },
  },
  prod: {
    keys: {
      NOTION_API_KEY: "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ",
    },
    sentry_env: "prod",
    client_download_urls: {
      macOS_x64:
        "https://fractal-chromium-macos-prod.s3.amazonaws.com/Whist.dmg",
      macOS_arm64:
        "https://fractal-chromium-macos-arm64-prod.s3.amazonaws.com/Whist.dmg",
      Windows:
        "https://fractal-chromium-windows-base.s3.amazonaws.com/Whist.exe",
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
