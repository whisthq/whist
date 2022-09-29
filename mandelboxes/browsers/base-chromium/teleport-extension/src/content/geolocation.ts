import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { injectResourceIntoDOM } from "@app/utils/dom"

const initLocationSpoofer = () => {
  injectResourceIntoDOM(document, "js/geolocation.js")

  // Listen for the creation of geolocation meta tags, name will be formatted as "<unique-id>-geolocation"
  const metaGeolocationObserver = new MutationObserver((mutationList, observer) => {
    mutationList.forEach((mutation) => {
      if (mutation.type === "childList") {
        mutation.addedNodes.forEach((addedNode) => {
          if (addedNode.nodeName === "META") {
            var metaTag = addedNode as HTMLMetaElement
            if (metaTag.name.search(/.*-geolocation/) > -1) {
              console.log("metatag with name " metaTag)
              const geolocationContent = JSON.parse(metaTag.content)
              chrome.runtime.sendMessage(<ContentScriptMessage>{
                type: ContentScriptMessageType.GEOLOCATION_REQUEST,
                value: {
                  params: geolocationContent,
                  metaTagName: metaTag.name
                },
              })
            }
          }
        })
      }
    })
  })
  metaGeolocationObserver.observe(document, {
    childList: true,
    subtree: true
  })
}

export { initLocationSpoofer }
