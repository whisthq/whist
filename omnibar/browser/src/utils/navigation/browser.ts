import { screen, BrowserWindow, BrowserView } from "electron";

import { createWindow, base } from "@app/utils/navigation";
import { WindowHashBrowserWindow } from "@app/utils/constants/app";

let browserWindow: BrowserWindow | undefined = undefined;

export const createBrowserWindow = () => {
  /*
    Create the primary "base" browser window

    Args:
        none
    Returns:
        win: BrowserWindow
   */
  const { width, height } = screen.getPrimaryDisplay().workAreaSize;

  browserWindow = createWindow({
    options: {
      ...base,
      width, // Full screen width
      height, // Full screen height
      id: WindowHashBrowserWindow,
      title: "", // Prevents default titlebar "Electron" text
      backgroundColor: "#ffffff",
      webPreferences: {
        partition: "persist:fractal",
        plugins: true,
        // TODO: enable sandbox, contextIsolation and disable nodeIntegration to improve security
        nodeIntegration: true,
        contextIsolation: false,
        javascript: true,
        // TODO: get rid of the remote module in renderers
        enableRemoteModule: true,
        worldSafeExecuteJavaScript: false,
      },
    },
    hash: WindowHashBrowserWindow,
    withReact: false,
  });

  browserWindow?.on("page-title-updated", (e) => {
    e.preventDefault();
  });

  browserWindow?.on("close", () => {
    try {
      const views = browserWindow?.getBrowserViews();

      if (views !== undefined && views.length > 0) {
        views.forEach((view: BrowserView) =>
          (view.webContents as any)?.destroy()
        );
      }
    } catch (err) {
      console.error(err);
    }

    browserWindow = undefined;
  });

  return browserWindow;
};

export const getBrowserWindow = () => browserWindow;

export const hideBrowserWindow = () => {
  browserWindow?.hide?.();
};

export const showBrowserWindow = (focus?: boolean) => {
  focus ?? true ? browserWindow?.show?.() : browserWindow?.showInactive();
  // If window is tiny, maximize. This can happen when we fullscreen an already fullscreened
  // window.
  const winSize = browserWindow?.getSize();
  if (
    winSize !== undefined &&
    (winSize[0] < 2 || winSize[1] < 30 || browserWindow?.isMinimized())
  ) {
    browserWindow?.maximize();
  }
};
