import {
  Menu,
  MenuItem,
  MenuItemConstructorOptions,
  globalShortcut,
} from "electron";
import EventEmitter from "events";
import { values } from "lodash";

// Electron shortcuts take callbacks instead of emitting events, so we need to make our own EventEmitter
export const shortcutEmitter = new EventEmitter();

interface Shortcut {
  label: string;
  accelerator: string;
  selector?: string;
}

// All the shortcuts
export const fractalShortcut = {
  OMNIBAR: { accelerator: "Option+A", label: "Toggle omnibar" },
  VIEW_1: { accelerator: "Option+1", label: "Open App 1" },
  VIEW_2: { accelerator: "Option+2", label: "Open App 2" },
  VIEW_3: { accelerator: "Option+3", label: "Open App 3" },
  VIEW_4: { accelerator: "Option+4", label: "Open App 4" },
  VIEW_5: { accelerator: "Option+5", label: "Open App 5" },
  CLOSE_TAB: { accelerator: "CommandOrControl+W", label: "Close current page" },
  NEW_TAB: { accelerator: "CommandOrControl+T", label: "Open new page" },
  REFRESH_TAB: {
    accelerator: "CommandOrControl+R",
    label: "Refresh current page",
  },
  GO_BACK: { accelerator: "Option+Backspace", label: "Go back in page" },
  GO_FORWARD: { accelerator: "Option+Enter", label: "Go forward in page" },
  FIND: { accelerator: "CommandorControl+F", label: "Find in page" },
  UNDO: { label: "Undo", accelerator: "CmdOrCtrl+Z", selector: "undo:" },
  REDO: { label: "Redo", accelerator: "Shift+CmdOrCtrl+Z", selector: "redo:" },
  CUT: { label: "Cut", accelerator: "CmdOrCtrl+X", selector: "cut:" },
  COPY: { label: "Copy", accelerator: "CmdOrCtrl+C", selector: "copy:" },
  PASTE: { label: "Paste", accelerator: "CmdOrCtrl+V", selector: "paste:" },
  SELECT_ALL: {
    label: "Select All",
    accelerator: "CmdOrCtrl+A",
    selector: "selectAll:",
  },
  HIDE: { label: "Hide browser window", accelerator: "Option+M" },
};

// Some shortcuts are global, this creates them
export const createGlobalShortcut = (shortcut: string) => {
  globalShortcut.register(shortcut, () => {
    shortcutEmitter.emit("shortcut-fired", shortcut);
  });
};

// Add all shortcuts to the menu
const submenu = values(fractalShortcut).map((shortcut: Shortcut) => ({
  label: shortcut.label,
  accelerator: shortcut.accelerator,
  ...(shortcut.selector !== undefined
    ? {
        selector: shortcut.selector,
      }
    : {
        click: () => {
          shortcutEmitter.emit("shortcut-fired", shortcut.accelerator);
        },
      }),
})) as MenuItemConstructorOptions[];

const menu = new Menu();
menu.append(
  new MenuItem({
    label: "Shortcuts",
    submenu: [...submenu, { role: "zoomIn" }, { role: "zoomOut" }],
  })
);

Menu.setApplicationMenu(menu);
