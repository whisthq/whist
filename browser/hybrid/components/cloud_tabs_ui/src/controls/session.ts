const initializeSessionStorage = () => {
  Storage.prototype.setObj = (key: string, obj: object) => {
    return sessionStorage.setItem(key, JSON.stringify(obj))
  }

  Storage.prototype.getObj = (key: string) => {
    const value = sessionStorage.getItem(key)
    return value === null ? value : JSON.parse(value)
  }

  Storage.prototype.setNumber = (key: string, value: number) => {
    return sessionStorage.setItem(key, value.toString())
  }

  Storage.prototype.getNumber = (key: string): number | null => {
    const value = sessionStorage.getItem(key)
    return value === null ? value : Number(value)
  }
}

const getTabId = () => sessionStorage.getNumber("tabId")

export { initializeSessionStorage, getTabId }
