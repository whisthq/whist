import { fromEvent } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import { importEvent } from "@app/utils/importer"
import TRIGGER from "@app/utils/triggers"

createTrigger(
  TRIGGER.cookiesImported,
  fromEvent(importEvent, "cookies-imported")
)
