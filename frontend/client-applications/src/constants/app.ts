// The session ID is a UNIX timestamp (milliseconds since epoch) that we use to track app
// launch times and also send to Amplitude and the host service to uniquely identify
// user sessions

export const sessionID = Date.now()

export const HEARTBEAT_INTERVAL_IN_MS = 5 * 60 * 1000
export const CHECK_UPDATE_INTERVAL_IN_MS = 5 * 60 * 1000

export const SENTRY_DSN =
  "https://5b0accb25f3341d280bb76f08775efe1@o400459.ingest.sentry.io/5412323"

export const MAX_URL_LENGTH = 2048
export const MAX_NEW_TAB_URLS = 1000

// Last updated: Jan 20, 2022
export const DEFAULT_LINUX_USER_AGENT =
  "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36"
export const DEFAULT_MAC_USER_AGENT =
  "Mozilla/5.0 (Macintosh; Intel Mac OS X 12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36"
export const DEFAULT_WINDOWS_USER_AGENT =
  "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36"
