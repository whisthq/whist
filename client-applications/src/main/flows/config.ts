import { map } from "rxjs/operators"
import { generateRandomConfigToken } from "@fractal/core-ts"
import { store } from "@app/utils/persist"
import { flow } from "@app/utils/flows"

export default flow<{
  userEmail: string
  accessToken: string
  refreshToken: string
}>("configFlow", (trigger) => {
  const withConfig = trigger.pipe(
    map((x) => ({
      ...x,
      configToken: store.get("auth.configToken") ?? generateRandomConfigToken(),
    }))
  )

  return {
    success: withConfig,
  }
})
