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

const rootMenu = allowPayments
  ? [
      {
        label: "Billing Information",
        click: () => {
          trayEvent.emit("payment")
        },
      },
      {
        label: "Sign out",
        click: () => {
          trayEvent.emit("signout")
        },
      },
      {
        label: "Quit",
        click: () => {
          trayEvent.emit("quit")
        },
      },
    ]
  : [
      {
        label: "Sign out",
        click: () => {
          trayEvent.emit("signout")
        },
      },
      {
        label: "Quit",
        click: () => {
          trayEvent.emit("quit")
        },
      },
    ]

const regionMenu = [
  {
    label: "(Admin Only) Region",
    submenu: values(defaultAllowedRegions).map((region: AWSRegion) => ({
      label: region,
      type: "radio",
      click: () => {
        trayEvent.emit("region", region)
      },
    })),
  } as unknown as MenuItem,
]

export const destroyTray = () => {
  tray?.destroy()
}

export const createTray = (email: string) => {
  // We should only have one tray at any given time
  if (tray != null) destroyTray()

  tray = new Tray(createNativeImage())
  // If the user is a @fractal.co developer, then allow them to toggle regions for testing
  const template = endsWith(email, "@fractal.co")
    ? [...rootMenu, ...regionMenu]
    : [...rootMenu]
  const menu = Menu.buildFromTemplate(template)
  tray.setContextMenu(menu)
}
