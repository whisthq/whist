import { app } from "electron"

// Disable smooth scrolling. If not disabled, this adds a ton of latency on top
// of our own scrolling logic
app.commandLine.appendSwitch("disable-smooth-scrolling")
