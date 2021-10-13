import { fromTrigger } from "@app/utils/flows"
import { aaronsCookieFunction, InstalledBrowser } from "@app/utils/importer"

fromTrigger("importerSubmitted").subscribe((payload: { browser: string }) => {
  if (payload.browser === "None") return

  aaronsCookieFunction(payload.browser as InstalledBrowser).catch((err) =>
    console.error(err)
  )
})
