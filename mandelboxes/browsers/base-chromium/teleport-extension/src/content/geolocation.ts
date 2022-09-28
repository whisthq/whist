import { injectResourceIntoDOM } from "@app/utils/dom"

const initLocationSpoofer = () => {
  injectResourceIntoDOM(document, "js/geolocation.js")

  // chrome.storage.sync.get(["geolocation"], (result) => {
  //   const meta_longitude = document.createElement("meta")
  //   meta_longitude.name = "longitude"
  //   meta_longitude.content = result.geolocation.longitude
  //   document.documentElement.appendChild(meta_longitude)

  //   const meta_latitude = document.createElement("meta")
  //   meta_latitude.name = "latitude"
  //   meta_latitude.content = result.geolocation.latitude
  //   document.documentElement.appendChild(meta_latitude)
  // })


  // // Create an empty geolocation DOM tag
  // metaGeolocation = document.createElement("meta")
  // metaGeolocation.name = "geolocation"
  // metaGeolocation.content = ""
  // document.documentElement.appendChild(metaGeolocation)

  // // Create an empty geolocation options DOM tag
  // metaGeolocationOptions = document.createElement("meta")
  // metaGeolocationOptions.name = "geolocation_options"
  // metaGeolocationOptions.content = ""
  // document.documentElement.appendChild(metaGeolocationOptions)

  // // Listen for changes to geolocation content
  // const metaGeolocationObserver = new MutationObserver((mutationList, observer) => {
  //   // This callback fires if there has been a change to the metaGeolocation.content attribute
  //   // TODO: tell worker to send message to client with tabID and options
  //   options = metaGeolocationOptions.content
  // })
  // metaGeolocationObserver.observe(metaGeolocation, {attributes: true, attributeFilter: ['content']})

  // Listen for the creation of geolocation meta tags, name will be formatted as "<unique-id>-geolocation"
  const metaGeolocationObserver = new MutationObserver((mutationList, observer) => {
    mutationList.forEach((mutation) => {
      if (mutation.type === "childList") {
        mutation.addedNodes.forEach((addedNode) => {
          if (addedNode.nodeName === "META") {
            var metaTag = addedNode as HTMLMetaElement
            if (metaTag.name.search(/.*-geolocation/) > -1) {
              const geolocationContent = JSON.parse(metaTag.content)
              chrome.runtime.sendMessage(<ContentScriptMessage>{
                type: ContentScriptMessageType.GEOLOCATION_REQUEST,
                value: isPointerLocked,
              })
            }
          }
        })
      }
    })
  })
  metaGeolocationObserver.observe(document.documentElement, {
    childList: true
    subtree: true
  })
}

export { initLocationSpoofer }
