const setStorage = (key: string, value: string) => {
  return new Promise<void>((resolve) => {
    chrome.storage.sync.set({ [key]: value }, () => {
      resolve()
    })
  })
}

const getStorage = (key: string) => {
  return new Promise((resolve) => {
    chrome.storage.sync.get([key], (result) => {
      resolve(result[key])
    })
  })
}

export { setStorage, getStorage }
