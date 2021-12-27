import { app, BrowserView } from "electron";
import { merge } from "rxjs";
import { withLatestFrom } from "rxjs/operators";
import { values, pick } from "lodash";

import { fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import {
  showOmnibar,
  hideOmnibar,
  omniBar,
  createOmnibar,
} from "@app/utils/navigation/omnibar";
import { navigate } from "@app/utils/navigation/url";
import {
  fractalShortcut,
  createGlobalShortcut,
} from "@app/utils/navigation/shortcuts";
import { destroyView } from "@app/utils/navigation/view";
import {
  getBrowserWindow,
  hideBrowserWindow,
  showBrowserWindow,
} from "@app/utils/navigation/browser";
import { showFinder } from "@app/utils/navigation/finder";

// Global shortcuts can be detected even when Fractal is in focus
fromTrigger(FractalTrigger.appReady).subscribe(() => {
  values(
    pick(fractalShortcut, [
      "OMNIBAR",
      "VIEW_1",
      "VIEW_2",
      "VIEW_3",
      "VIEW_4",
      "VIEW_5",
      "VIEW_6",
      "VIEW_7",
      "VIEW_8",
      "VIEW_9",
      "HIDE",
    ])
  ).forEach((shortcut: { accelerator: string }) => {
    createGlobalShortcut(shortcut.accelerator);
  });
});

fromTrigger(FractalTrigger.shortcutFired).subscribe((shortcut: string) => {
  switch (shortcut) {
    case fractalShortcut.OMNIBAR.accelerator: {
      if (omniBar === undefined) {
        createOmnibar();
        showOmnibar();
      } else {
        omniBar?.isVisible() ?? false ? hideOmnibar() : showOmnibar();
      }
      break;
    }
    case fractalShortcut.VIEW_1.accelerator: {
      hideOmnibar();
      navigate("https://www.gmail.com");
      break;
    }
    case fractalShortcut.VIEW_2.accelerator: {
      hideOmnibar();
      navigate("https://www.google.com/calendar");
      break;
    }
    case fractalShortcut.FIND.accelerator: {
      const win = getBrowserWindow();
      if (win === undefined) break;
      showFinder(win);
      break;
    }
    case fractalShortcut.HIDE.accelerator: {
      const win = getBrowserWindow();
      if (win === undefined) break;
      if (!win?.isFocused() || !win?.isVisible()) {
        showBrowserWindow();
      } else {
        hideOmnibar();
        hideBrowserWindow();
      }
      break;
    }
    default: {
      break;
    }
  }
});

fromTrigger(FractalTrigger.shortcutFired)
  .pipe(
    withLatestFrom(
      merge(
        fromTrigger(FractalTrigger.viewCreated),
        fromTrigger(FractalTrigger.viewDisplayed)
      )
    )
  )
  .subscribe(([shortcut, view]: [string, BrowserView]) => {
    switch (shortcut) {
      case fractalShortcut.CLOSE_TAB.accelerator: {
        const win = getBrowserWindow();
        destroyView(view, win);
        break;
      }
      case fractalShortcut.NEW_TAB.accelerator: {
        showOmnibar();
        break;
      }
      case fractalShortcut.REFRESH_TAB.accelerator: {
        view.webContents?.reload();
        break;
      }
      case fractalShortcut.GO_BACK.accelerator: {
        view.webContents?.goBack();
        break;
      }
      case fractalShortcut.GO_FORWARD.accelerator: {
        view.webContents?.goForward();
        break;
      }
      default: {
        break;
      }
    }
  });
