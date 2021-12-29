import { app } from "electron"
import { fromEvent } from "rxjs"

const appReady = fromEvent(app, "ready")

export { appReady }
