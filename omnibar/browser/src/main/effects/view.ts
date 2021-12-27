import { BrowserView, clipboard, Menu, MenuItem } from "electron";
import { take } from "rxjs/operators";

import { fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import {
  getView,
  displayView,
  destroyView,
  createView,
  attachView,
} from "@app/utils/navigation/view";
import {
  createBrowserWindow,
  getBrowserWindow,
} from "@app/utils/navigation/browser";

// Sometimes, a URL will redirect after you open it. This makes sure the redirected URL
// isn't already open; if it is, we destroy the new view and swap out the existing one
fromTrigger(FractalTrigger.viewCreated).subscribe((view: BrowserView) => {
  const win = getBrowserWindow();

  const alreadyOpen = getView(view.webContents?.id, win);

  if (
    alreadyOpen != null &&
    view.webContents?.id !== alreadyOpen.webContents?.id
  ) {
    displayView(alreadyOpen, win);
    destroyView(view, win);
  }
});

// Create context menu (i.e. right click popup with copy, inspect element, etc.)
fromTrigger(FractalTrigger.viewCreated).subscribe((view: BrowserView) => {
  view.webContents.on("context-menu", (e, params) => {
    e.preventDefault();

    const menuItems = [
      { label: "Go back", click: () => view.webContents?.goBack() },
      { label: "Go forward", click: () => view.webContents?.goForward() },
      { label: "Reload", click: () => view.webContents?.reload() },
      {
        label: "Copy page URL",
        click: () => clipboard.writeText(params.pageURL),
      },
      ...((params.selectionText ?? "") !== "" ? [{ role: "copy" }] : []),
      ...((params.srcURL ?? "") !== ""
        ? [
            {
              label: "Copy image",
              click: () => view.webContents?.copyImageAt(params.x, params.y),
            },
            {
              label: "Copy image address",
              click: () => clipboard.writeText(params.srcURL),
            },
          ]
        : []),
      { type: "separator" },
      {
        label: "Inspect Element",
        click: () => {
          view.webContents?.inspectElement(params.x, params.y);
        },
      },
    ];

    const menu = new Menu();
    menuItems.forEach((item: object) => {
      const menuItem = new MenuItem(item);
      menu.append(menuItem);
    });

    menu.popup();
  });
});

// Sometimes a URL will look valid but not actually load. If so, convert to a Google search instead
fromTrigger(FractalTrigger.viewFailed)
  .pipe(take(1))
  .subscribe((view: BrowserView) => {
    const win = getBrowserWindow();
    const url = view.webContents?.getURL();

    const newWin = getBrowserWindow() ?? createBrowserWindow();
    const _view = createView(
      `https://www.google.com/search?q=${url.replace("https://www.", "")}`
    );
    attachView(_view, newWin);

    // For some reason, Electron crashes if we destroy the view too quickly
    setTimeout(() => destroyView(view, win), 5000);
  });
