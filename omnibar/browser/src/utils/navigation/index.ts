/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file contains utility functions to create Electron windows. This file manages
 * creation of renderer process windows. It is called from the main process, and passes all
 * the configuration needed to load files into Electron renderer windows.
 */

import path from "path";
import { app, BrowserWindow } from "electron";

const buildRoot = app.isPackaged
  ? path.join(app.getAppPath(), "build")
  : path.resolve("public");

// Parameters passed into Electron's createWindow() function
export const base = {
  webPreferences: {
    preload: path.join(buildRoot, "preload.js"),
    partition: "persist:fractal",
  },
};
// Window dimensions
export const width = {
  xs: { width: 16 * 24 },
  sm: { width: 16 * 32 },
  md: { width: 16 * 40 },
  lg: { width: 16 * 48 },
  xl: { width: 16 * 64 },
  xl2: { width: 16 * 80 },
  xl3: { width: 16 * 96 },
};

export const height = {
  xs: { height: 16 * 4 },
  sm: { height: 16 * 20 },
  md: { height: 16 * 44 },
  lg: { height: 16 * 56 },
  xl: { height: 16 * 64 },
  xl2: { height: 16 * 80 },
  xl3: { height: 16 * 96 },
};

export const createWindow = (args: {
  options: object;
  hash: string;
  withReact: boolean;
  onReady?: (win: BrowserWindow) => void;
}) => {
  /*
    Creates any Electron window

    Args:
      options (object): See Electron BrowserWindow docs for all options
      hash (string): Unique identifier for the window
      withReact (boolean): If true, load the renderer thread, else load a URL
      onReady ((win: BrowserWindow) => void): Function to run when window is loaded

    Returns:
      win: BrowserWindow
  */

  const win = new BrowserWindow({ ...args.options });

  // Electron doesn't have a API for passing data to renderer windows. We need
  // to pass "init" data for a variety of reasons, but mainly so we can decide
  // which React component to render into the window. We're forced to do this
  // using query parameters in the URL that we pass. The alternative would
  // be to use separate index.html files for each window, which we want to avoid.
  const params = `?show=${args.hash}`;

  if (app.isPackaged && args.withReact) {
    win
      .loadFile("build/index.html", { search: params })
      .catch((err) => console.log(err));
  } else if (args.withReact) {
    win
      .loadURL(`http://localhost:8080${params}`)
      .catch((err) => console.error(err));
  }

  // We accept some callbacks in case the caller needs to run some additional
  // functions on open/close.
  // Electron recommends showing the window on the ready-to-show event:
  // https://www.electronjs.org/docs/api/browser-window
  win.once("ready-to-show", () => {
    args?.onReady?.(win);
  });

  return win;
};

export const getWindow = (id: string) => {
  const filteredWindows = BrowserWindow.getAllWindows().filter(
    (win: BrowserWindow) => {
      const webContents = win.webContents as unknown as {
        browserWindowOptions: { id: string };
      };
      return webContents?.browserWindowOptions?.id?.toString() === id;
    }
  );
  if (filteredWindows.length > 0) return filteredWindows[0];
  return undefined;
};
