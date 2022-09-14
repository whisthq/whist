export enum CloudTabError {
  IS_INVALID_URL = "Whoops! Due to an unexpected error, we were unable to enable this cloud tab. Opening this URL in a different tab may fix this issue.",
  IS_INVALID_SCHEME = "For performance and privacy reasons, cloud tabs are only enabled for sites with http: or https: URLs.",
  IS_VIDEO = "Since videos are already streamed, we don't recommend turning this site into a cloud tab; doing so may lead to worse video quality.",
  IS_UNSUPPORTED_URL = "Whoops! Cloud tabs are disabled for this site because we've detected that it's not compatible with cloud streaming.",
  IS_LOCALHOST = "It looks like you're running localhost! Localhost should be kept local, so we've disabled cloud tabs on this site.",
  IS_VIDEO_CHAT = "Whoops! It looks like this site has video/audio conferencing. To ensure the best quality, we've disabled cloud tabs on this site.",
  IS_WHIST_URL = "Whoops! It looks like you're on a Whist internal page, which cannot be streamed.",
  IS_INCOGNITO = "It looks like you're incognito. For added privacy, cloud tabs have been disabled for all incognito windows.",
  MULTIWINDOW_NOT_SUPPORTED = "Sorry! Whist currently only supports cloud tabs within a single window, and you have a cloud tab running in another window.",
}
