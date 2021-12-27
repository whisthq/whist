import { app, BrowserWindow } from "electron";
import { fromEvent } from "rxjs";
import { map } from "rxjs/operators";

import { fromTrigger, createTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { viewEmitter } from "@app/utils/navigation/view";

// Window created trigger
createTrigger(
  FractalTrigger.browserWindowCreated,
  fromEvent(app, "browser-window-created")
);
// Window closed trigger
fromTrigger(FractalTrigger.browserWindowCreated).subscribe(
  ([, win]: [any, BrowserWindow]) => {
    createTrigger(FractalTrigger.browserWindowClosed, fromEvent(win, "closed"));
  }
);
// Window will close trigger
fromTrigger(FractalTrigger.browserWindowCreated).subscribe(
  ([, win]: [any, BrowserWindow]) => {
    createTrigger(
      FractalTrigger.browserWindowWillClose,
      fromEvent(win, "close").pipe(map((event: any) => [event, win]))
    );
  }
);

createTrigger(
  FractalTrigger.newWindowRedirected,
  fromEvent(viewEmitter, "new-window-redirected")
);

createTrigger(
  FractalTrigger.windowsAllClosed,
  fromEvent(app, "window-all-closed")
);
