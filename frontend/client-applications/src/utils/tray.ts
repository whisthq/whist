/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file auth.ts
 * @brief This file contains utility functions for managing the MacOS tray, which appears at the
 * top right corner of the screen when the protocol launches.
 */

import events from "events"
import { Menu, Tray, nativeImage } from "electron"

import { trayIconPath } from "@app/config/files"
import { AWSRegion } from "@app/@types/aws"
import { defaultAllowedRegions } from "@app/constants/mandelbox"
import { MenuItem } from "electron/main"
import { createLicenseWindow, createSpeedtestWindow } from "@app/utils/windows"
import { persistGet } from "@app/utils/persist"
import {
  RESTORE_LAST_SESSION,
  WHIST_IS_DEFAULT_BROWSER,
} from "@app/constants/store"
import { openSourceUrls } from "@app/constants/app"

// We create the tray here so that it persists throughout the application
let tray: Tray | null = null
export const trayEvent = new events.EventEmitter()

const createNativeImage = () => {
  const image = nativeImage?.createFromPath(trayIconPath)?.resize({ width: 14 })
  image.setTemplateImage(true)
  return image
}

const aboutMenu = new MenuItem({
  label: "About",
  submenu: [
    {
      label: "Open Source Licenses",
      click: () => {
        openSourceUrls.forEach((url) => createLicenseWindow(url))
      },
    },
  ],
})

const feedbackMenu = new MenuItem({
  label: "Support",
  submenu: [
    {
      label: "Feedback",
      click: () => {
        trayEvent.emit("feedback")
      },
    },
    {
      label: "Bug Report",
      click: () => {
        trayEvent.emit("bug")
      },
    },
    {
      label: "Test My Internet Speed",
      click: () => {
        createSpeedtestWindow()
      },
    },
  ],
})

const regionMenu = new MenuItem({
  label: "(Admin Only)",
  submenu: Object.values(defaultAllowedRegions).map((region: AWSRegion) => ({
    label: region,
    type: "radio",
    click: () => {
      trayEvent.emit("region", region)
    },
  })),
})

const accountMenu = new MenuItem({
  label: "Account",
  submenu: [
    {
      label: "Billing Information",
      click: () => {
        trayEvent.emit("payment")
      },
    },

    {
      label: "Sign Out",
      click: () => {
        trayEvent.emit("signout")
      },
    },
  ],
})

const settingsMenu = new MenuItem({
  label: "Settings",
  submenu: [
    {
      label: "Restore the last browser session",
      type: "checkbox",
      checked: <boolean>persistGet(RESTORE_LAST_SESSION) ?? true,
      click: () => {
        trayEvent.emit("restore-last-browser-session")
      },
    },
    {
      label: "Make Whist my default browser",
      type: "checkbox",
      checked: <boolean>persistGet(WHIST_IS_DEFAULT_BROWSER) ?? false,
      click: () => {
        trayEvent.emit("whist-is-default-browser")
      },
    },
  ],
})

const destroyTray = () => {
  tray?.destroy()
}

const createTray = (menu: Menu) => {
  // We should only have one tray at any given time
  if (tray != null) destroyTray()
  // Set the tray icon
  tray = new Tray(createNativeImage())
  // Create the contents of the tray (i.e. menu)
  tray.setContextMenu(menu)
}

const createMenu = (signedIn: boolean, userEmail?: string) =>
  Menu.buildFromTemplate([
    ...(signedIn ? [accountMenu] : []),
    ...[settingsMenu, feedbackMenu, aboutMenu],
    ...(signedIn && (userEmail ?? "").endsWith("@whist.com")
      ? [regionMenu]
      : []),
  ])

export { destroyTray, createTray, createMenu }
