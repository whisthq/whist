import path from "path"
import Store from "electron-store"
import { of } from "rxjs"
import { spawn, ChildProcess } from "child_process"

import authFlow from "@app/main/flows/auth"
import mandelboxFlow from "@app/main/flows/mandelbox"

// PROTOCOL LAUNCH STUFF
const protocolPath = path.join(
  __dirname,
  "../..",
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

const writeStream = (process: ChildProcess | undefined, message: string) => {
  process?.stdin?.write?.(message)
}

const serializePorts = (ps: {
  port_32262: number
  port_32263: number
  port_32273: number
}) => `32262:${ps?.port_32262}.32263:${ps?.port_32263}.32273:${ps?.port_32273}`

const protocolStreamInfo = (
  info: {
    mandelboxIP: string
    mandelboxSecret: string
    mandelboxPorts: {
      port_32262: number
      port_32263: number
      port_32273: number
    }
  },
  protocol: ChildProcess
) => {
  if (protocol === undefined) return
  writeStream(
    protocol,
    `ports?${serializePorts(info.mandelboxPorts)}\nprivate-key?${
      info.mandelboxSecret
    }\nip?${info.mandelboxIP}\nfinished?0\n`
  )
}

const authCache = {
  userEmail: "ming@fractal.co",
  accessToken:
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjV5NXRJTGpxRW9PS3JPS3pEYlMxeCJ9.eyJodHRwczovL2FwaS5mcmFjdGFsLmNvL3N0cmlwZV9jdXN0b21lcl9pZCI6ImN1c19KZmRFMG9LMG9QcnJSTyIsImh0dHBzOi8vYXBpLmZyYWN0YWwuY28vc3Vic2NyaXB0aW9uX3N0YXR1cyI6ImFjdGl2ZSIsImlzcyI6Imh0dHBzOi8vZnJhY3RhbC1kZXYudXMuYXV0aDAuY29tLyIsInN1YiI6Imdvb2dsZS1vYXV0aDJ8MTA1NTQ2NzQ0MTExMzk4MTA3Mjg1IiwiYXVkIjpbImh0dHBzOi8vYXBpLmZyYWN0YWwuY28iLCJodHRwczovL2ZyYWN0YWwtZGV2LnVzLmF1dGgwLmNvbS91c2VyaW5mbyJdLCJpYXQiOjE2MzE5MDExNDksImV4cCI6MTYzMTk4NzU0OSwiYXpwIjoiajF0b2lmcEtWdTVXQTZZYmhFcUJuTVdOMGZGeHF3NUkiLCJzY29wZSI6Im9wZW5pZCBwcm9maWxlIGVtYWlsIGFkbWluIG9mZmxpbmVfYWNjZXNzIn0.RWPFzMAW-aY6WC7uHOrV-gTD6N7uNOtSB5_Y5KQfOq9HR4TczwFGtD3n9hOHOiPqk95sPCM4h2rDs2Z48BePYDbU-rIomLFF_RPV9KNAxLOnu4Bn-XMTE1md1COWGNVpX8v1kQ5TCGhfnyroQOZoBVIj1ln9xhI_8IMiIrF6G_SAyBub9OeX5jHYVSYqhu7xAYYZQ2leiYG_LXDtjuGvPE3Ep5P468gTbsqi5ev3XGD95lheUxVyVleThNZDxCYHcTjKw51R61ZwtTbXosvIeLQK4ZeYtyTBJ3eUWM1f7SUuuCF0-oxpw85GV1OGT-2Iy3OxMkUthkYs2LU82624kg",
  refreshToken: "LL0rYM2GSdXabhg9txLAR4quZIsrLQ96o1PO9l1NJRweF",
  configToken:
    "UwkdeyKuwNymjgxFlakEH1pKK95tOfeF7xJd3XniKx8goQqYFGZaHB6O28z4XX0y",
}

const auth = authFlow(of(authCache))

const mandelbox = mandelboxFlow(auth.success)

mandelbox.success.subscribe(
  (info: {
    mandelboxIP: string
    mandelboxSecret: string
    mandelboxPorts: {
      port_32262: number
      port_32263: number
      port_32273: number
    }
  }) => {
    console.log("mandelboxflow success")
    protocolStreamInfo(info, protocol)
  }
)

auth.success.subscribe(() => {
  console.log("auth flow worked")
})
