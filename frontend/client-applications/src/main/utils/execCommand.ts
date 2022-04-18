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
    return ""
  }
}

export { execCommand }
