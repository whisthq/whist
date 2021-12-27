import { app } from "electron";
import { fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";

import {
  getBrowserWindow,
  showBrowserWindow,
} from "@app/utils/navigation/browser";
import { navigate } from "@app/utils/navigation/url";
import { hideOmnibar } from "@app/utils/navigation/omnibar";
import { fromSignal } from "@app/utils/rxjs/observables";

// If a new window opens up, instead keep it in the same window as a new tab
fromTrigger(FractalTrigger.newWindowRedirected).subscribe((url: string) => {
  navigate(url);
  hideOmnibar();
});

fromSignal(
  fromTrigger(FractalTrigger.openUrlDefault),
  fromTrigger(FractalTrigger.appReady)
).subscribe(([e, url]: [any, string]) => {
  e.preventDefault();
  navigate(url);
  hideOmnibar();
});

// fromTrigger(FractalTrigger.appReady).subscribe(() => {
//   app.requestSingleInstanceLock();

//   app.on("second-instance", (e) => {
//     e.preventDefault();
//     // Someone tried to run a second instance, we should focus our window.
//     const win = getBrowserWindow();
//     if (!win?.isVisible()) showBrowserWindow();
//   });
// });

// fromTrigger(FractalTrigger.windowsAllClosed).subscribe((event: any) => {
//   event?.preventDefault();
//   app?.dock?.hide();
// });
