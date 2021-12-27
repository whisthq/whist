import { ipcMain } from "electron";
import { Observable, fromEvent } from "rxjs";
import { filter, map, pluck, startWith, share } from "rxjs/operators";

import { StateChannel } from "@app/utils/constants/app";
import { StateIPC } from "@app/utils/constants/state";
import { createTrigger, fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";

createTrigger(
  FractalTrigger.eventIPC,
  fromEvent(ipcMain, StateChannel).pipe(
    map((args) => {
      if (!Array.isArray(args)) return {} as Partial<StateIPC>;
      return args[1] as Partial<StateIPC>;
    }),
    startWith({}),
    share()
  )
);

const filterByName = (
  observable: Observable<{ name: string; payload: any }>,
  name: string
) =>
  observable.pipe(
    filter(
      (x: { name: string; payload: any }) => x !== undefined && x.name === name
    ),
    map((x: { name: string; payload: any }) => x.payload)
  );

[
  FractalTrigger.createView,
  FractalTrigger.displayView,
  FractalTrigger.changeViewUrl,
  FractalTrigger.closeView,
  FractalTrigger.finderEnterPressed,
  FractalTrigger.finderUp,
  FractalTrigger.finderDown,
  FractalTrigger.finderClose,
  FractalTrigger.doneOnboarding,
  FractalTrigger.importChromeConfig,
].forEach((trigger: string) => {
  createTrigger(
    trigger,
    filterByName(
      fromTrigger("eventIPC" as FractalTrigger).pipe(pluck("trigger")),
      trigger
    )
  );
});
