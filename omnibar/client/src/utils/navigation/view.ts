import { BrowserView, BrowserWindow } from "electron";
import EventEmitter from "events";

// There's no global view-created, view-deleted events so we create our own
export const viewEmitter = new EventEmitter();

export const attachView = (view: BrowserView, window: BrowserWindow) => {
  window.addBrowserView(view);
  view.setBounds({
    x: 0,
    y: 0,
    width: window.getSize()[0],
    height: window.getSize()[1] - 20,
  });
  view.setAutoResize({
    width: true,
    height: true,
    horizontal: true,
    vertical: true,
  });
};

export const createView = (url: string) => {
  /*
        Creates a View and attaches it to a browser window (i.e. displays it in the foreground)

        Args:
            url (string): URL to display
            window (BrowserWindow): Window to attach the View to

        Returns:
            view: BrowserView
    */
  let didLoad = false;

  const options = {
    webPreferences: {
      partition: "persist:fractal",
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: true,
      enableRemoteModule: false,
      plugins: true,
      nativeWindowOpen: true,
      webSecurity: true,
      javascript: true,
      worldSafeExecuteJavaScript: false,
    },
  };

  const view = new BrowserView(options);
  view.setBackgroundColor("#ffffff");
  view.webContents?.loadURL(url);

  view.webContents?.on("did-navigate", () => {
    didLoad = true;
    viewEmitter.emit("view-created", view);
  });

  view.webContents?.on("did-fail-load", () => {
    if (!didLoad) viewEmitter.emit("view-failed", view);
  });

  view.webContents?.on("page-title-updated", () => {
    viewEmitter.emit("view-title-changed");
  });

  view.webContents.addListener(
    "new-window",
    (e, url, frameName, disposition) => {
      if (
        disposition === "new-window" &&
        ["_self", "_blank"].includes(frameName)
      ) {
        e.preventDefault();
        viewEmitter.emit("new-window-redirected", url);
      } else if (
        disposition === "foreground-tab" ||
        disposition === "background-tab"
      ) {
        e.preventDefault();
        viewEmitter.emit("new-window-redirected", url);
      }
    }
  );

  return view;
};

export const displayView = (view: BrowserView, win?: BrowserWindow) => {
  if (win === undefined) return;
  // Set view to top
  win.setTopBrowserView(view);
  // Emit event
  viewEmitter.emit("view-displayed", view);
};

export const getView = (id: number, win?: BrowserWindow) => {
  if (win === undefined) return undefined;

  const existingViews = win.getBrowserViews().filter((view: BrowserView) => {
    return view.webContents?.id === id;
  });

  return existingViews.length > 0 ? existingViews[0] : undefined;
};

export const destroyView = (view: BrowserView, win?: BrowserWindow) => {
  if (win === undefined) return;
  // Display another BrowserView. We need to do this because Electron's API doesn't provide us
  // a way of knowing which BrowserView is on top, so we need to control it manually.
  const views = win.getBrowserViews();
  views.forEach((_view: BrowserView, index: number) => {
    if (_view.webContents.id === view.webContents.id) {
      if (views.length > 1 && index === 0) {
        displayView(views[1], win);
      } else if (views.length > 1) {
        displayView(views[index - 1], win);
      }
    }
  });
  // Remove the BrowserView from the BrowserWindow. If there are no more BrowserViews remaining,
  // close the window
  win.getBrowserViews().length > 1 ? win?.removeBrowserView(view) : win.close();
  // Removing the BrowserView doesn't actually destroy it; this does
  (view.webContents as any)?.destroy();
  viewEmitter.emit("view-destroyed");
};
