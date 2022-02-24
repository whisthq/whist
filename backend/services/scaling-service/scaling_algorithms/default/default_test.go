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

func TestCreateBuffer(t *testing.T) {
	testAlgo := DefaultScalingAlgorithm{}

	// Check that the buffer is in its zero value
	ok := reflect.DeepEqual(testAlgo.InstanceBuffer, 0)
	if !ok {
		t.Errorf("Instance buffer is not zero value. Got %v, want %v", testAlgo.InstanceBuffer, 0)
	}

	testAlgo.CreateBuffer()
	ok = reflect.DeepEqual(testAlgo.InstanceBuffer, DEFAULT_INSTANCE_BUFFER)
	if !ok {
		t.Errorf("Instance buffer was not set correctly. Got %v, want %v", testAlgo.InstanceBuffer, DEFAULT_INSTANCE_BUFFER)
	}

}
