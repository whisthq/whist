// The session ID is a UNIX timestamp (milliseconds since epoch) that we use to track app
// launch times and also send to Amplitude and the host service to uniquely identify
// user sessions

export const sessionID = Date.now()
