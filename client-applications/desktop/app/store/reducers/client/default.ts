import { OperatingSystem } from "shared/types/client"
import { AWSRegion } from "shared/types/aws"

export type Timer = {
    appOpened?: number
    appDoneLoading?: number
    createContainerRequestSent?: number
    protocolLaunched?: number
    protocolClosed?: number
}

export type ComputerInfo = {
    operatingSystem?: OperatingSystem | undefined
    region?: AWSRegion | undefined
}

export type ClientDefault = {
    timer: Timer
    computerInfo: ComputerInfo
}

export const DEFAULT: ClientDefault = {
    timer: {
        appOpened: 0,
        appDoneLoading: 0,
        createContainerRequestSent: 0,
        protocolLaunched: 0,
        protocolClosed: 0,
    },
    computerInfo: {
        operatingSystem: undefined,
        region: undefined,
    },
}
