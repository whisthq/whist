import { app, BrowserWindow, screen } from "electron";

import { createWindow, base, width, height } from "@app/utils/navigation";
import { WindowHashOmnibar } from "@app/utils/constants/app";

// This is the search bar, summoned via Shift+Space+F
export let omniBar: BrowserWindow | undefined;

export const createOmnibar = () => {
  omniBar = createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.sm,
      frame: false, // Frameless, no traffic light controls or titlebar
      transparent: true, // Transparent background
      alwaysOnTop: true,
      resizable: false,
      fullscreenable: false,
      minimizable: false,
      titleBarStyle: "hidden",
    },
    hash: WindowHashOmnibar,
    withReact: true,
  });

  omniBar.on("close", () => {
    omniBar = undefined;
  });

  omniBar.setWindowButtonVisibility(false);

  // omniBar.webContents.toggleDevTools();
};

export const hideOmnibar = () => {
  omniBar?.hide();
};

export const showOmnibar = () => {
  omniBar?.setAlwaysOnTop(true, "floating");
  omniBar?.show();
  omniBar?.focus();
};
