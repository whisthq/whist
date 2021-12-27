/**
 * Copyright Fractal Computers, Inc. 2021
 * @file logging.ts
 * @brief This file contains utility functions for logging.
 */

import { app } from "electron";
import util from "util";
import fs from "fs";
import path from "path";

export const electronLogPath = path.join(app.getPath("userData"), "logs");

// Open a file handle to append to the logs file.
// Create the loggingBaseFilePath directory if it does not exist.
const openLogFile = () => {
  fs.mkdirSync(electronLogPath, { recursive: true });
  const logPath = path.join(electronLogPath, "electron.log");
  return fs.createWriteStream(logPath);
};

const logFile = openLogFile();

export const localLog = (logs: string | object) => {
  if (!app.isPackaged) console.log(logs);
  logFile.write(util.inspect(logs));
};
