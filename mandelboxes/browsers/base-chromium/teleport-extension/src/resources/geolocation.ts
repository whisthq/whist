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
  geoLocation: {
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
      latitude: geoLocation.latitude,
      longitude: geoLocation.longitude,
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
  let geoLocation = getGeolocation() as
    | undefined
    | { longitude: number; latitude: number }

  if (geoLocation === undefined) {
    const observer = new MutationObserver((_mutations, obs) => {
      geoLocation = getGeolocation()

      if (geoLocation !== undefined) {
        spoofLocation(geoLocation, successCallback)
        obs.disconnect()
      }
    })

    observer.observe(document, {
      childList: true,
      subtree: true,
    })
  } else {
    spoofLocation(geoLocation, successCallback)
  }
}
