import { config } from "@app/constants/app"

export const videoUrls = [
  "youtube.com",
  "tiktok.com",
  "vimeo.com",
  "twitch.com",
  "dailymotion.com",
  "netflix.com",
  "hulu.com",
  "hbomax.com",
  "disneyplus.com",
  "espn.com",
  "tennistv.com",
]
export const videoChatUrls = ["meet.google.com", "gather.town", "zoom.us"]
export const unsupportedUrls = ["abercrombie.com"]
export const whistUrls = [config.AUTH0_REDIRECT_URL, config.AUTH0_DOMAIN_URL]
export const whistLoadingUrl =
  "data:text/plain;charset=utf-8;base64,V2hpc3QgaXMgbG9hZGluZw=="
