const path = require('path');
const { app } = require('electron');
const fs = require('fs-extra');
const appName = app.getName();

// Get app directory
const appPath = path.join(app.getPath('appData'), appName);

// Recursively delete all app files
fs.rmdirSync(appPath, { recursive: true });
app.exit();
