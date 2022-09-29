const getUniqueId = () => {
  return Date.now().toString()
}

// In a cloud tab:
// 1. Site requests location - 
//     > call one of navigator.geolocation functions
// 2. Server extension tells client extension that there is a location request
//     > create invisible DOM and listen for it in the content
// 3. Call relevant geolocation function locally
// 3a. Client extension requests user for location permission via webui (todo: security origin location permission difference)
// 3b. Location permission is provided or denied
// 4. Geolocation is sent from client extension to server extension
// 5. Worker (which is also listening on socket) messages geolocation content script (tab-specific)
// 6. Content script inserts geolocation information into the appropriate (tab-specific) DOM - can be serialized JSON of what client returned
// 7. navigator.geolocation.getCurrentPosition has a listener for specific DOM change (see step 6) with .then that calls the appropriate callback

// NOTE: avoid race conditions by using counter/ID - track and respond to individual geolocation commands via a unique ID (timestamp?)


// TODO: also override watchCurrentPosition and clearWatch
navigator.geolocation.getCurrentPosition = (
  successCallback,
  _errorCallback,
  _options
) => {
  const uniqueId = getUniqueId()

  const metaGeolocation = document.createElement("meta")
  metaGeolocation.name = `${uniqueId}-geolocation`
  metaGeolocation.content = JSON.stringify({
    function: "getCurrentPosition",
    options: _options,
  })
  document.documentElement.appendChild(metaGeolocation)
}
