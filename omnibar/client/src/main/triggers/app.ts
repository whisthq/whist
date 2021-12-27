import { app } from "electron";
import { fromEvent } from "rxjs";

import { createTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";

// Fires when Electron starts; this is the first event to fire
createTrigger(FractalTrigger.appReady, fromEvent(app, "ready"));

// Fires when Electron is used as a default browser for opening URLs
createTrigger(FractalTrigger.openUrlDefault, fromEvent(app, "open-url"));
