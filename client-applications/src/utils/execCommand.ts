/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file execCommand.ts
 * @brief This file contains constant functions to execute commands.
 */

// Execute a command in a cross-platform manner.
import { execSync } from "child_process"

export const execCommand = (command, cwd, env = {}, stdio = "inherit") => {
  try {
    const output = execSync(command, {
      encoding: "utf-8",
      cwd: cwd,
      env: { ...process.env, ...env },
      stdio: stdio,
    })
    return output
  } catch (error) {
    return null
  }
}

export const execCommandByOS = (
  macCommand,
  linuxCommand,
  windowsCommand,
  cwd,
  env = {},
  stdio = "inherit"
) => {
  const currentPlatform = process.platform
  if (currentPlatform === "darwin") {
    if (macCommand != null) {
      return execCommand(macCommand, cwd, env, stdio)
    } else {
      return null
    }
  } else if (currentPlatform === "win32" || currentPlatform === "win64") {
    if (windowsCommand != null) {
      return execCommand(windowsCommand, cwd, env, stdio)
    } else {
      return null
    }
  } else if (currentPlatform === "linux") {
    if (linuxCommand != null) {
      return execCommand(linuxCommand, cwd, env, stdio)
    } else {
      return null
    }
  } else {
    return null
  }
}
