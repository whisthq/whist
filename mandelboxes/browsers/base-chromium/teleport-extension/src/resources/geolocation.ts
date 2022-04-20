navigator.geolocation.getCurrentPosition = (
  successCallback,
  errorCallback,
  options
) => {
  successCallback({
    coords: {
      accuracy: 15.0,
      altitude: null,
      altitudeAccuracy: null,
      heading: null,
      latitude: 40.0,
      longitude: 40.0,
      speed: null,
    } as GeolocationCoordinates,
    timestamp: Date.now(),
  } as GeolocationPosition)
}
