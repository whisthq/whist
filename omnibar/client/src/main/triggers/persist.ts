import { fromEvent } from "rxjs";

import { createTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { persisted } from "@app/utils/state/persist";

createTrigger(
  FractalTrigger.persistDidChange,
  fromEvent(persisted, "did-change")
);
