/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file contains utility functions for managing the MacOS tray, which appears at the
 * top right corner of the screen when the protocol launches.
 */

import events from "events";
import { Menu, Tray, nativeImage, app } from "electron";
import path from "path";
import { values, endsWith } from "lodash";
import { MenuItem } from "electron/main";

const buildRoot = app.isPackaged
  ? path.join(app.getAppPath(), "build")
  : path.resolve("public");

const trayIconMac = "assets/images/trayIconBlackTemplate.png";
const trayIconWindows = "assets/images/trayIconPurple.ico";
const trayIconPath =
  process.platform === "win32"
    ? path.join(buildRoot, trayIconWindows)
    : path.join(buildRoot, trayIconMac);

// We create the tray here so that it persists throughout the application
let tray: Tray | null = null;
export const trayEvent = new events.EventEmitter();

const createNativeImage = () => {
  const image = nativeImage
    ?.createFromPath(trayIconPath)
    ?.resize({ width: 14 });
  image.setTemplateImage(true);
  return image;
};

const quitMenu = new MenuItem({
  label: "Quit Fractal",
  click: () => {
    destroyTray();
    app.quit();
  },
});

export const destroyTray = () => {
  tray?.destroy();
};

export const createTray = () => {
  // We should only have one tray at any given time
  if (tray != null) destroyTray();
  const menu = Menu.buildFromTemplate([...[quitMenu]]);

  // Set the tray icon
  tray = new Tray(createNativeImage());
  // Create the contents of the tray (i.e. menu)
  tray.setContextMenu(menu);
};
