import { fromEvent } from "rxjs"

import { createTrigger } from "@app/main/utils/flows"
import { window } from "@app/main/utils/renderer"
import { WhistTrigger } from "@app/constants/triggers"

createTrigger(WhistTrigger.authInfo, fromEvent(window, "auth-info"))
