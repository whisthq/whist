navigator.geolocation.getCurrentPosition = (
  successCallback,
  errorCallback,
  options
) => {
  setTimeout(() => {
    const meta_longitude = document.documentElement.querySelector(
      'meta[name="longitude"]'
    ) as HTMLMetaElement
    const meta_latitude = document.documentElement.querySelector(
      'meta[name="latitude"]'
    ) as HTMLMetaElement

    const latitude = (meta_latitude?.content as unknown as string) ?? undefined
    const longitude =
      (meta_longitude?.content as unknown as string) ?? undefined

    if (latitude !== undefined && longitude !== undefined) {
      successCallback({
        coords: {
          accuracy: 15.0,
          altitude: null,
          altitudeAccuracy: null,
          heading: null,
          latitude: parseFloat(latitude),
          longitude: parseFloat(longitude),
          speed: null,
        } as GeolocationCoordinates,
        timestamp: Date.now(),
      } as GeolocationPosition)
    }

    meta_longitude.remove()
    meta_latitude.remove()
  }, 1500)
}
