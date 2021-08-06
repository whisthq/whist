/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file contains utility functions for managing the MacOS tray, which appears at the
 * top right corner of the screen when the protocol launches.
 */

import events from "events"
import { Menu, Tray, nativeImage } from "electron"
import { values, endsWith } from "lodash"

import { trayIconPath } from "@app/config/files"
import { AWSRegion, defaultAllowedRegions } from "@app/@types/aws"
import { allowPayments } from "@app/utils/constants"
import { MenuItem } from "electron/main"

// We create the tray here so that it persists throughout the application
let tray: Tray | null = null
export const trayEvent = new events.EventEmitter()

const createNativeImage = () => {
  const image = nativeImage?.createFromPath(trayIconPath)?.resize({ width: 14 })
  image.setTemplateImage(true)
  return image
}

const feedbackMenu = new MenuItem({
  label: "Leave feedback",
  click: () => {
    trayEvent.emit("feedback")
  },
})

const regionMenu = new MenuItem({
  label: "(Admin Only)",
  submenu: values(defaultAllowedRegions).map((region: AWSRegion) => ({
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
      label: "(Coming Soon) Make Fractal my default browser",
      click: () => {
        trayEvent.emit("defaultBrowser")
      },
    },
  ],
})

export const destroyTray = () => {
  tray?.destroy()
}

export const createTray = (menu: Menu) => {
  // We should only have one tray at any given time
  if (tray != null) destroyTray()
  // Set the tray icon
  tray = new Tray(createNativeImage())
  // Create the contents of the tray (i.e. menu)
  tray.setContextMenu(menu)
}

export const createMenu = (signedIn: boolean, userEmail?: string) =>
  Menu.buildFromTemplate([
    ...(signedIn && allowPayments ? [accountMenu] : []),
    ...[settingsMenu, feedbackMenu],
    ...(signedIn && endsWith(userEmail ?? "", "@fractal.co")
      ? [regionMenu]
      : []),
  ])
