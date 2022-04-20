const getGeolocation = () => {
  const meta_longitude = document.documentElement.querySelector(
    'meta[name="longitude"]'
  ) as HTMLMetaElement
  const meta_latitude = document.documentElement.querySelector(
    'meta[name="latitude"]'
  ) as HTMLMetaElement

  const latitude = (meta_latitude?.content as unknown as string) ?? undefined
  const longitude = (meta_longitude?.content as unknown as string) ?? undefined

  if (latitude !== undefined && longitude !== undefined) {
    const longitude = parseFloat(meta_longitude.content)
    const latitude = parseFloat(meta_latitude.content)

    return {
      longitude,
      latitude,
    }
  }
  return undefined
}

const spoofLocation = (
  geolocation: {
    longitude: number
    latitude: number
  },
  successCallback: (position: GeolocationPosition) => void
) => {
  successCallback({
    coords: {
      accuracy: 15.0,
      altitude: null,
      altitudeAccuracy: null,
      heading: null,
      latitude: geolocation.latitude,
      longitude: geolocation.longitude,
      speed: null,
    } as GeolocationCoordinates,
    timestamp: Date.now(),
  } as GeolocationPosition)
}

navigator.geolocation.getCurrentPosition = (
  successCallback,
  _errorCallback,
  _options
) => {
  let geolocation = getGeolocation() as
    | undefined
    | { longitude: number; latitude: number }

  if (geolocation === undefined) {
    const observer = new MutationObserver((_mutations, obs) => {
      geolocation = getGeolocation()

      if (geolocation !== undefined) {
        spoofLocation(geolocation, successCallback)
        obs.disconnect()
      }
    })

    observer.observe(document, {
      childList: true,
      subtree: true,
    })
  } else {
    spoofLocation(geolocation, successCallback)
  }
}
