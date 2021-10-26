import { fromTrigger } from "@app/utils/flows"
import { getDecryptedCookies, InstalledBrowser } from "@app/utils/importer"
import { hostImportConfig } from "@app/utils/host"

fromTrigger("importerSubmitted").subscribe(
  async (payload: { browser: string }) => {
    if (payload.browser === "None") return

    const cookies = await getDecryptedCookies(
      payload.browser as InstalledBrowser
    )
  }
)
