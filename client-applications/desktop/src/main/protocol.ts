import { app } from "electron"
import {
    protocolLaunch,
    protocolStreamInfo,
    protocolStreamKill,
} from "@app/utils/protocol"
import {
    containerInfoIP,
    containerInfoPorts,
    containerInfoSecretKey,
} from "@app/utils/container"
import {
    containerAssignRequest,
    containerAssignSuccess,
    containerAssignFailure,
} from "@app/main/container"
import { merge, zip, of } from "rxjs"
import { mapTo, map, share, exhaustMap } from "rxjs/operators"

export const protocolLaunchRequest = containerAssignRequest.pipe(
    exhaustMap(() => of(protocolLaunch())),
    share()
)
export const protocolLaunchLoading = protocolLaunchRequest.pipe(
    exhaustMap(() => merge(of(true), containerAssignSuccess.pipe(mapTo(false))))
)

export const protocolLaunchSuccess = containerAssignSuccess.pipe(
    map((res) => ({
        ip: containerInfoIP(res),
        secret_key: containerInfoSecretKey(res),
        ports: containerInfoPorts(res),
    }))
)

export const protocolLaunchFailure = containerAssignFailure

// Close the application when the protocol closes.
protocolLaunchRequest.subscribe((protocol) =>
    protocol.on("close", () => app.exit())
)

// Stream the ip, secret_key, and ports info to the protocol when we
// when we receive a successful container status response.
zip(
    protocolLaunchRequest,
    protocolLaunchSuccess
).subscribe(([protocol, info]) => protocolStreamInfo(protocol, info))

// If we have an error, close the protocol.
zip(
    protocolLaunchRequest,
    protocolLaunchFailure
).subscribe(([protocol, _error]) => protocolStreamKill(protocol))
