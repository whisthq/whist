import { injectResourceIntoDOM } from "@app/utils/dom"

const initLocationSpoofer = () => {
  injectResourceIntoDOM(document, "js/geolocation.js")

  chrome.storage.local.get(["geolocation"], (result) => {
    console.log("Local storage got", result)

    const meta_longitude = document.createElement("meta")
    meta_longitude.name = "longitude"
    meta_longitude.content = result.geolocation.longitude
    document.documentElement.appendChild(meta_longitude)

    const meta_latitude = document.createElement("meta")
    meta_latitude.name = "latitude"
    meta_latitude.content = result.geolocation.latitude
    document.documentElement.appendChild(meta_latitude)
  })
}

export { initLocationSpoofer }
