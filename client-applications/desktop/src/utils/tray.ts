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
  const image = nativeImage?.createFromPath(trayIconPath)?.resize({ width: 16 })
  image.setTemplateImage(true)
  return image
}

const feedbackMenu = new MenuItem({
  label: "Leave feedback",
  click: () => {
    trayEvent.emit("feedback")
  },
})

const signoutMenu = new MenuItem({
  id: "signout",
  label: "Sign out",
  click: () => {
    trayEvent.emit("signout")
  },
  enabled: false,
})
const quitMenu = new MenuItem({
  id: "quit",
  label: "Quit",
  click: () => {
    trayEvent.emit("quit")
  },
})
const regionMenu = new MenuItem({
  label: "(Admin Only) Region",
  submenu: values(defaultAllowedRegions).map((region: AWSRegion) => ({
    label: region,
    type: "radio",
    click: () => {
      trayEvent.emit("region", region)
    },
  })),
})

const paymentMenu = new MenuItem({
  label: "Billing Information",
  click: () => {
    trayEvent.emit("payment")
  },
})

export const destroyTray = () => {
  tray?.destroy()
}

export const createTray = () => {
  // We should only have one tray at any given time
  if (tray != null) destroyTray()

  tray = new Tray(createNativeImage())
  const menu = Menu.buildFromTemplate([feedbackMenu, signoutMenu, quitMenu])
  tray.setContextMenu(menu)
}

// Function to update the tray after the user has logged in
export const toggleSignedInTray = (userEmail: string) => {
  signoutMenu.enabled = true
  const menu = Menu.buildFromTemplate([feedbackMenu, signoutMenu, quitMenu])
  // If the user is a @fractal.co developer, then allow them to toggle regions for testing
  if (endsWith(userEmail, "@fractal.co")) {
    menu.append(regionMenu)
  }
  if (allowPayments) {
    menu.insert(0, paymentMenu)
  }
  tray?.setContextMenu(menu)
}

export const toggleSignedOutTray = () => {
  signoutMenu.enabled = false
  const menu = Menu.buildFromTemplate([feedbackMenu, signoutMenu, quitMenu])
  tray?.setContextMenu(menu)
}
