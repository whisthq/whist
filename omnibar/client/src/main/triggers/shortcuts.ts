import { fromEvent } from "rxjs";

import { createTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { shortcutEmitter } from "@app/utils/navigation/shortcuts";

createTrigger(
  FractalTrigger.shortcutFired,
  fromEvent(shortcutEmitter, "shortcut-fired")
);
