import { of } from "rxjs"
import { filter } from "rxjs/operators"

import { message } from "@app/worker/utils/messaging"
import { OPEN_GOOGLE_AUTH } from "@app/constants/messaging"

const openGoogleAuth = message.pipe(
  filter(({ request }) => request.message === OPEN_GOOGLE_AUTH)
)

const shouldAuthenticate = of(undefined)

export { openGoogleAuth, shouldAuthenticate }
