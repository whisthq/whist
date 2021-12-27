import { BrowserView } from "electron";
import { merge } from "rxjs";
import { filter, map, take, withLatestFrom } from "rxjs/operators";

import { fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { createFinder, hideFinder } from "@app/utils/navigation/finder";
import { getBrowserWindow } from "@app/utils/navigation/browser";

fromTrigger(FractalTrigger.appReady).subscribe(() => {
  createFinder();
});

fromTrigger(FractalTrigger.finderClose).subscribe(() => {
  const win = getBrowserWindow();
  hideFinder(win);
});

const findRequest = merge(
  fromTrigger(FractalTrigger.finderEnterPressed).pipe(
    map((payload: { text: string }) => ({ text: payload.text, forward: true }))
  ),
  fromTrigger(FractalTrigger.finderDown).pipe(
    map((payload: { text: string }) => ({ text: payload.text, forward: true }))
  ),
  fromTrigger(FractalTrigger.finderUp).pipe(
    map((payload: { text: string }) => ({ text: payload.text, forward: false }))
  )
).pipe(
  withLatestFrom(
    merge(
      fromTrigger(FractalTrigger.viewCreated),
      fromTrigger(FractalTrigger.viewDisplayed)
    )
  ),
  filter(
    ([payload]: [{ text: string; forward: boolean }, BrowserView]) =>
      (payload.text ?? "") !== ""
  )
);

findRequest.subscribe(
  ([payload, view]: [{ text: string; forward: boolean }, BrowserView]) => {
    view?.webContents?.findInPage(payload.text, { forward: payload.forward });
  }
);

findRequest
  .pipe(take(1))
  .subscribe(([, view]: [{ text: string; forward: boolean }, BrowserView]) => {
    view?.webContents?.on("found-in-page", () => {
      view?.webContents?.stopFindInPage("keepSelection");
    });
  });
