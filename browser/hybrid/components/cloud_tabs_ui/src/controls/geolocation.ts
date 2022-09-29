const intializeGeolocationRequestHandler = () => {
  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (
      (parsed.type === "GEOLOCATION_REQUESTED") &&
      parsed.value.id === getTabId()
    ) {
      const requestedFunction = parsed.value.params.function
      const serverMetaTagName = parsed.value.metaTagName
      switch(requestedFunction) {
        case 'getCurrentPosition':
          navigator.geolocation.getCurrentPosition(
            (position: Position) => {
              // Send message back to server with position
              ;(chrome as any).whist.broadcastWhistMessage(
                JSON.stringify({
                  type: "GEOLOCATION_REQUEST_SUCCESS",
                  value: {
                    position: position,
                    metaTagName: serverMetaTagName,
                    tabId: parsed.value.id
                  },
                })
              )
            },
            (positionError: PositionError) => {
              // Send message back to server with failure
              ;(chrome as any).whist.broadcastWhistMessage(
                JSON.stringify({
                  type: "GEOLOCATION_REQUEST_FAILURE",
                  value: {
                    positionError: positionError,
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