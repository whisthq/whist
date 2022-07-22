const toBase64 = (url: string) => {
  const promise = new Promise<string>((resolve) => {
    var xhr = new XMLHttpRequest()
    xhr.onload = () => {
      var reader = new FileReader()
      reader.onloadend = () => {
        resolve(reader.result as any)
      }
      reader.readAsDataURL(xhr.response)
    }
    xhr.open("GET", url)
    xhr.responseType = "blob"
    xhr.send()
  })

  return promise
}

export { toBase64 }
