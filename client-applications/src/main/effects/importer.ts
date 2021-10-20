import { fromTrigger } from "@app/utils/flows"
import { getDecryptedCookies, InstalledBrowser } from "@app/utils/importer"

fromTrigger("importerSubmitted").subscribe((payload: { browser: string }) => {
  console.log(payload)

  if (payload.browser === "None") return

  console.log("getting decrypted cookies")

  getDecryptedCookies(payload.browser as InstalledBrowser).catch((err) =>
    console.error(err)
  )
})
