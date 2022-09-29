import { getTabId } from "./session"

const intializeGeolocationRequestHandler = () => {
  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    console.log(parsed)

    if (
      (parsed.type === "GEOLOCATION_REQUESTED") &&
      parsed.value.id === getTabId()
    ) {
      const requestedFunction = parsed.value.params.function
      const serverMetaTagName = parsed.value.metaTagName
      switch(requestedFunction) {
        case 'getCurrentPosition':
          navigator.geolocation.getCurrentPosition(
            (position: GeolocationPosition) => {
              // Send message back to server with position
              ;(chrome as any).whist.broadcastWhistMessage(
                JSON.stringify({
                  type: "GEOLOCATION_REQUEST_COMPLETED",
                  value: {
                    success: true,
                    response: position,
                    metaTagName: serverMetaTagName,
                    tabId: parsed.value.id
                  },
                })
              )
            },
            (positionError: GeolocationPositionError) => {
              // Send message back to server with failure
              ;(chrome as any).whist.broadcastWhistMessage(
                JSON.stringify({
                  type: "GEOLOCATION_REQUEST_COMPLETED",
                  value: {
                    success: false,
                    response: positionError,
                    metaTagName: serverMetaTagName,
                    tabId: parsed.value.id
                  },
                })
              )
            },
            parsed.value.options // TODO: do we need to check if options exists?
          )
      }
    }
  })
}

export {
  intializeGeolocationRequestHandler,
}