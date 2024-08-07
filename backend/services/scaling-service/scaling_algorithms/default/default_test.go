package scaling_algorithms

import (
	"reflect"
	"testing"
)

func TestCreateEventChans(t *testing.T) {
	testAlgo := DefaultScalingAlgorithm{}
	testAlgo.CreateEventChans()

	// Send and receive some test events
	// to confirm the channels were created
	instanceEvent := ScalingEvent{
		Data:   "test-data-instance-event",
		Region: "test-region",
	}
	testAlgo.InstanceEventChan <- instanceEvent

	imageEvent := ScalingEvent{
		Data:   12345,
		Region: "test-region",
	}
	testAlgo.ImageEventChan <- imageEvent

	scheduledEvent := ScalingEvent{
		Data:   true,
		Region: "test-region",
	}
	testAlgo.ScheduledEventChan <- scheduledEvent

	// Now receive from each channel and validate
	gotInstanceEvent := <-testAlgo.InstanceEventChan
	ok := reflect.DeepEqual(gotInstanceEvent, instanceEvent)
	if !ok {
		t.Errorf("Got the wrong event from the instance event chan. Got %v, want %v", gotInstanceEvent, instanceEvent)
	}

	gotImageEvent := <-testAlgo.ImageEventChan
	ok = reflect.DeepEqual(gotImageEvent, imageEvent)
	if !ok {
		t.Errorf("Got the wrong event from the image event chan. Got %v, want %v", gotImageEvent, imageEvent)
	}

	gotScheduledEvent := <-testAlgo.ScheduledEventChan
	ok = reflect.DeepEqual(gotScheduledEvent, scheduledEvent)
	if !ok {
		t.Errorf("Got the wrong event from the scheduled event chan. Got %v, want %v", gotInstanceEvent, instanceEvent)
	}
}
