import { fromEventPattern } from "rxjs"

const setStorage = (key: string, value: string) => {
  return new Promise<void>((resolve) => {
    chrome.storage.local.set({ [key]: value }, () => {
      resolve()
    })
  })
}

const getStorage = (key: string) => {
  return new Promise((resolve) => {
    chrome.storage.local.get([key], (result) => {
      resolve(result[key])
    })
  })
}

const storageDidChange = fromEventPattern(
  (handler: any) => chrome.storage.onChanged.addListener(handler),
  (handler: any) => chrome.storage.onChanged.removeListener(handler),
  (changes: object, areaName: string) => ({
    changes,
    areaName,
  })
)

export { getStorage, setStorage, storageDidChange }
