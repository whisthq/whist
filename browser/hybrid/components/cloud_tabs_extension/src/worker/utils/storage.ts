import { fromEventPattern, ReplaySubject } from "rxjs"
import { map, filter, share } from "rxjs/operators"

import { Storage } from "@app/constants/storage"

const setStorage = async (key: string, value: any) => {
  return await new Promise<void>((resolve) => {
    chrome.storage.local.set({ [key]: value }, () => {
      resolve()
    })
  })
}

const getStorage = async <T>(key: string) => {
  const promise: Promise<T> = new Promise((resolve) => {
    chrome.storage.local.get([key], (result) => {
      resolve(result[key])
    })
  })

  return await promise
}

const storageSubject = new ReplaySubject<Record<string, any>>()

const storageChanged = fromEventPattern(
  (handler: any) => chrome.storage.onChanged.addListener(handler),
  (handler: any) => chrome.storage.onChanged.removeListener(handler),
  (changes: object, _areaName: string) => changes
).pipe(share())

storageChanged.subscribe((changes: object) => storageSubject.next(changes))

const storageDidChange = (key: Storage) =>
  storageSubject.pipe(
    filter((changes: Record<string, any>) => changes[key] !== undefined),
    map((changes: Record<string, any>) => {
      const oldValue = changes[key].oldValue
      const newValue = changes[key].newValue

      return { oldValue, newValue }
    }),
    share()
  )

export { getStorage, setStorage, storageDidChange }
