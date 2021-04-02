import { fromPersisted } from "@app/main/events"
import { map } from "rxjs/operators"

export const userEmail = fromPersisted("userEmail").pipe(
    map((s) => s as string)
)
export const userConfigToken = fromPersisted("userConfigToken").pipe(
    map((s) => s as string)
)
export const userAccessToken = fromPersisted("userAccessToken").pipe(
    map((s) => s as string)
)
