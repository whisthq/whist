import { fromTrigger } from "@app/utils/flows"
import { getDecryptedCookies, InstalledBrowser } from "@app/utils/importer"

fromTrigger("importerSubmitted").subscribe((payload: { browser: string }) => {
  if (payload.browser === "None") return

  getDecryptedCookies(payload.browser as InstalledBrowser).catch((err) =>
    console.error(err)
  )
})
