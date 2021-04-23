const path = require('path');
const { app } = require('electron');
const fs = require('fs-extra');
const appName = app.getName();

// Get app directory -- "Electron" for local development, "Fractal" for packaged
const devAppPath = path.join(app.getPath('appData'), "Electron");
const prodAppPath = path.join(app.getPath('appData'), "Fractal");

// Recursively delete all app files
fs.rmdirSync(devAppPath, { recursive: true });
fs.rmdirSync(prodAppPath, { recursive: true });
app.exit();
