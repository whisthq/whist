import { fromEvent } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import { auth0Event } from "@app/utils/auth"
import TRIGGER from "@app/utils/triggers"

createTrigger(TRIGGER.authInfo, fromEvent(auth0Event, "auth-info"))
