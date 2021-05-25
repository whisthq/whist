import events from "events"
import { Menu, Tray, nativeImage } from "electron"
import { values } from "lodash"

import { trayIconPath } from "@app/config/files"
import { AWSRegion, defaultAllowedRegions } from "@app/@types/aws"

// We create the tray here so that it persists throughout the application
let tray: Tray | null = null
export const trayEvent = new events.EventEmitter()

const createNativeImage = () => {
  let image = nativeImage.createFromPath(trayIconPath)
  image = image.resize({ width: 16 })
  image.setTemplateImage(true)
  return image
}

const rootMenu = [
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

export const createTray = () => {
  // We should only have one tray at any given time
  if (tray != null) tray.destroy()

  tray = new Tray(createNativeImage())
  const menu = Menu.buildFromTemplate([
    ...rootMenu,
    {
      label: "Region",
      submenu: [
        {
          label: AWSRegion.CA_CENTRAL_1,
          type: "radio",
          click: () => {
            trayEvent.emit("region", AWSRegion.CA_CENTRAL_1)
          },
        },
        {
          label: AWSRegion.US_EAST_1,
          type: "radio",
          click: () => {
            trayEvent.emit("region", AWSRegion.US_EAST_1)
          },
        },
        {
          label: AWSRegion.US_WEST_1,
          type: "radio",
          click: () => {
            trayEvent.emit("region", AWSRegion.US_WEST_1)
          },
        },
        {
          label: AWSRegion.EU_CENTRAL_1,
          type: "radio",
          click: () => {
            trayEvent.emit("region", AWSRegion.EU_CENTRAL_1)
          },
        },
      ],
    },
  ])
  tray.setContextMenu(menu)
}
