import { fromEventPersist } from "@app/main/events"
import { map } from "rxjs/operators"

export const userEmail = fromEventPersist("userEmail").pipe(
    map((s) => s as string)
)
export const userConfigToken = fromEventPersist("userConfigToken").pipe(
    map((s) => s as string)
)
export const userAccessToken = fromEventPersist("userAccessToken").pipe(
    map((s) => s as string)
)
