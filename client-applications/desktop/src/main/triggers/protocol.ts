import { fromEvent } from "rxjs"

import { childProcess } from "@app/utils/protocol"
import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

createTrigger(TRIGGER.childProcessSpawn, fromEvent(childProcess, "spawn"))
createTrigger(TRIGGER.childProcessClose, fromEvent(childProcess, "close"))
