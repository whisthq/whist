export enum FractalAppStates {
    PENDING = "PENDING", // is spinning up
    READY = "READY", // successfully spun up, ready to connect
    FAILURE = "FAILURE", // failed to spin up, cannot be connected to
    CANCELLED = "CANCELLED", // has been cancelled (treated as a FAILURE effectivelly)
    NULL = "NULL", // does not exist or was never spun up
}

export enum FractalStatus {
    SUCCESS = "SUCCESS",
    FAILURE = "FAILURE",
}
