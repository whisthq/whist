import { fromEventPattern } from "rxjs"

const storageDidChange = fromEventPattern(
  (handler: any) => chrome.storage.onChanged.addListener(handler),
  (handler: any) => chrome.storage.onChanged.removeListener(handler),
  (changes: object, areaName: string) => ({
    changes,
    areaName,
  })
)

export { storageDidChange }
