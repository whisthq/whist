import { BrowserWindow, BrowserView, app } from "electron";

import { base, getWindow } from "@app/utils/navigation";
import { WindowHashFinder } from "@app/utils/constants/app";
import { destroyView } from "@app/utils/navigation/view";

let finder: BrowserView | undefined = undefined;

export const createFinder = () => {
  /*
    Create the primary "base" browser window
    Args:
        none
    Returns:
        win: BrowserWindow
   */
  const options = {
    ...base,
    resizable: false,
    frame: false,
    id: WindowHashFinder,
    alwaysOnTop: true,
    show: false,
    transparent: true,
    shadow: false,
    rounded: false,
    movable: false,
    minimizable: false,
    fullscreenable: false,
    roundedCorners: true,
  };

  finder = new BrowserView(options);

  finder?.setBackgroundColor("#ffffff");

  const params = `?show=${WindowHashFinder}`;

  if (app.isPackaged) {
    finder.webContents
      .loadFile("build/index.html", { search: params })
      .catch((err: any) => console.log(err));
  } else {
    finder.webContents
      .loadURL(`http://localhost:8080${params}`)
      .catch((err: any) => console.error(err));
  }

  return finder;
};

export const getFinder = () => finder;

export const showFinder = (window?: BrowserWindow) => {
  if (window === undefined || finder === undefined) return;

  window.addBrowserView(finder);
  window.setTopBrowserView(finder);

  const winCoordinates = window.getPosition();
  const winSize = window.getSize();

  finder?.setBounds({
    x: winCoordinates[0] + winSize[0] - 350,
    y: winCoordinates[1] + 5,
    width: 325,
    height: 70,
  });
};

export const hideFinder = (win?: BrowserWindow) => {
  if (finder !== undefined) win?.removeBrowserView(finder);
};

export const destroyFinder = (win?: BrowserWindow) => {
  if (finder !== undefined) destroyView(finder, win);
};
