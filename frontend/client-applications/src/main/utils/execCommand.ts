/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file execCommand.ts
 * @brief This file contains constant functions to execute commands.
 */

// Execute a command in a cross-platform manner.
import { execSync, StdioOptions } from "child_process"

const execCommand = (
  command: string,
  cwd: string,
  env = {},
  stdio = "inherit"
) => {
  try {
    const output = execSync(command, {
      encoding: "utf-8",
      cwd: cwd,
      env: { ...process.env, ...env },
      stdio: <StdioOptions>stdio,
    })
    return output
  } catch (error) {
    return null
  }
}

export const execCommandByOS = (
  macCommand: string,
  linuxCommand: string,
  windowsCommand: string,
  cwd: string,
  env = {},
  stdio = "inherit"
) => {
  const currentPlatform = process.platform
  if (currentPlatform === "darwin") {
    if (macCommand !== "") {
      return execCommand(macCommand, cwd, env, stdio)
    } else {
      return null
    }
  } else if (currentPlatform === "win32") {
    if (windowsCommand !== "") {
      return execCommand(windowsCommand, cwd, env, stdio)
    } else {
      return null
    }
  } else if (currentPlatform === "linux") {
    if (linuxCommand !== "") {
      return execCommand(linuxCommand, cwd, env, stdio)
    } else {
      return null
    }
  } else {
    return null
  }
}
