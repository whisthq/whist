package main

// These indicate the action a scaling event will perform.
const (
	SpinupInstance      = "SPIN_UP_INSTANCE_FOR_ALLOCATED_MANDELBOX"
	ScaleDownInstances  = "TRY_SCALE_DOWN_IF_NECESSARY"
	MarkInstanceAsReady = "MARK_INSTANCE_AS_READY_ON_DATABASE"
	VerifyScaleDown     = "VERIFY_DRAINING_INSTANCE_TERMINATION"
	CreateImageBuffer   = "CREATE_IMAGE_BUFFER"
)

// These indicate the type of event, according to the source it originated from.
const (
	DatabaseEvent = "DATABASE_EVENT"
	TimingEvent   = "TIMING_EVENT"
)
