import { sendGAEvent } from "@app/utils/analytics"
import {
  protocolLaunchProcess,
} from "@app/main/observables/protocol"
import { loginSuccess } from "@app/main/observables/login"
import { signupSuccess } from "@app/main/observables/signup"
import { protocolCloseRequest } from "@app/main/observables/protocol"

enum GAEvent {
    PROTOCOL_LAUNCH = "Protocol launch",
    PROTOCOL_CLOSE = "Protocol close",
    SIGNUP = "Signup"
}

sendGAEvent(
    GAEvent.PROTOCOL_LAUNCH,
    loginSuccess,
    signupSuccess,
    protocolLaunchProcess
)

sendGAEvent(
    GAEvent.PROTOCOL_CLOSE,
    protocolCloseRequest
)

sendGAEvent(
    GAEvent.SIGNUP,
    signupSuccess
)