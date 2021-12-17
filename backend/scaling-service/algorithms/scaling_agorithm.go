package algorithms

type ScalingAlgorithm interface {
	createEventChan() chan Event
}

type Event struct {
	Name    string
	Details string
}
