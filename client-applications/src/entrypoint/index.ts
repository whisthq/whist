import { spawn } from "child_process"
import path from "path"

const protocolPath = path.join(
  __dirname,
  "../../../",
  "protocol-build",
  "client",
  "./_Fractal"
)

const protocolParameters = {
  environment: "development",
}

const protocolArguments = [
  ...Object.entries(protocolParameters)
    .map(([flag, arg]) => [`--${flag}`, arg])
    .flat(),
  "--read-pipe",
]

const protocol = spawn(protocolPath, protocolArguments, {
  detached: false,
  // options are for [stdin, stdout, stderr]. pipe creates a pipe, ignore will ignore the
  // output. We pipe stdin since that's how we send args to the protocol. We pipe stdout
  // and later use that pipe to write logs to `protocol.log` and potentially stdout.
  stdio: ["pipe", "pipe", "ignore"],
})

console.log("Spawned", protocol)
