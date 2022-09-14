const toBase64 = async (url: string) => {
  const promise = new Promise<string>((resolve) => {
    const xhr = new XMLHttpRequest()
    xhr.onload = () => {
      const reader = new FileReader()
      reader.onloadend = () => {
        resolve(reader.result as any)
      }
      reader.readAsDataURL(xhr.response)
    }
    xhr.open("GET", url)
    xhr.responseType = "blob"
    xhr.send()
  })

  return await promise
}

export { toBase64 }
