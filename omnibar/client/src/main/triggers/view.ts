import { fromEvent } from "rxjs";

import { createTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { viewEmitter } from "@app/utils/navigation/view";

createTrigger(
  FractalTrigger.viewCreated,
  fromEvent(viewEmitter, "view-created")
);
createTrigger(
  FractalTrigger.viewDisplayed,
  fromEvent(viewEmitter, "view-displayed")
);
createTrigger(
  FractalTrigger.viewDestroyed,
  fromEvent(viewEmitter, "view-destroyed")
);
createTrigger(FractalTrigger.viewFailed, fromEvent(viewEmitter, "view-failed"));
createTrigger(
  FractalTrigger.viewTitleChanged,
  fromEvent(viewEmitter, "view-title-changed")
);
