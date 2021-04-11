/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import { zip } from "rxjs"
import {
    protocolStreamInfo,
    protocolStreamKill,
    protocolFolder,
} from "@app/utils/protocol"
import {
    protocolLaunchProcess,
    protocolLaunchSuccess,
    protocolLaunchFailure,
    protocolCloseRequest,
} from "@app/main/observables/protocol"
import { userEmail } from "@app/main/observables/user"
import { uploadToS3 } from "@app/utils/logging"
import { app } from "electron"

// Streaming information to protocol
// Stream the ip, secret_key, and ports info to the protocol when we
// when we receive a successful container status response.
zip(
    protocolLaunchProcess,
    protocolLaunchSuccess
).subscribe(([protocol, info]) => protocolStreamInfo(protocol, info))

// Protocol closing
// If we have an error, close the protocol.
zip(
    protocolLaunchProcess,
    protocolLaunchFailure
).subscribe(([protocol, _error]) => protocolStreamKill(protocol))

// gets userEmail from observable to pass in to s3 upload 
let email = ""
userEmail.subscribe((e: any) => email = e)

protocolCloseRequest.subscribe(async () => {
    await uploadToS3(email)
    app.quit()
})
