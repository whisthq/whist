// State of container creation from GraphQL
export enum FractalAppState {
    PENDING = "PENDING", // existing container being assigned
    NO_TASK = "NO_TASK", // no Celery task has been created yet (container/assign not called)
    READY = "READY", // successfully spun up, ready to connect
    FAILURE = "FAILURE", // failed to spin up, cannot be connected to
    CANCELLED = "CANCELLED", // has been cancelled (treated as a FAILURE effectivelly)
    SPINNING_UP_NEW = "SPINNING_UP_NEW", // new container being started
}

// State of task status from /status endpoint
export enum FractalTaskStatus {
    SUCCESS = "SUCCESS",
    FAILURE = "FAILURE",
    PENDING = "PENDING",
}
