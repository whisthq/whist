// Suppress startup pages
user_pref("browser.startup.page", 0)
user_pref("datareporting.policy.firstRunURL", "")

// Enable General Hardware Acceleration
user_pref("layers.acceleration.force-enabled", true)
user_pref("gfx.webrender.all", true)

// Enable Hardware Video Acceleration
// https://wiki.archlinux.org/index.php/Firefox#Hardware_video_acceleration
user_pref("media.ffmpeg.vaapi.enabled", true)
user_pref("media.ffmpeg.vaapi-drm-display.enabled", true)
user_pref("media.ffvpx.enabled", false)
user_pref("media.rdd-vpx.enabled", false)
