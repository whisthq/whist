import { fromEvent } from "rxjs"

import { createTrigger } from "@app/main/utils/flows"
import { auth0Event } from "@app/main/utils/windows"
import { WhistTrigger } from "@app/constants/triggers"

createTrigger(WhistTrigger.authInfo, fromEvent(auth0Event, "auth-info"))
