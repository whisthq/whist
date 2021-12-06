package utils

import (
	"time"
	"testing"
)

// TestStopAndDrainTimer will confirm no deadlocks occurred and timerChan is not empty
func TestStopAndDrainTimer(t *testing.T) {
	// Simulate the actual usage of this function
	timerChan := make(chan bool)
	receivedTimerChan := false
	sleepTime := 0 * time.Millisecond
	timer := time.AfterFunc(time.Duration(sleepTime), func() { timerChan <- true })

	// Gives a `timerChan`ce for the channel to be filled
	receivedTimerChan = <-timerChan

	// Does nothing as timer has been stopped
	StopAndDrainTimer(timer)

	// Stop should return false to indicate the timer has already been stopped
	if timer.Stop() {
		t.Fatal("error stopping and draining timer. Expected stop() to return false, go true")
	}

	// receivedTimerChan should be true
	if !receivedTimerChan {
		t.Fatal("error stopping and draining timer. Expected receivedTimerChan: true, got false")
	}
}

// TestStopAndDrainTimerAlreadyStopped will confirm no deadlocks occurred and timerChan is empty
func TestStopAndDrainTimerAlreadyStopped(t *testing.T) {
	// Simulate the actual usage of this function
	timerChan := make(chan bool)
	sleepTime := 60000
	timer := time.AfterFunc(time.Duration(sleepTime)*time.Millisecond, func() { timerChan <- true })

	//  Will stop timer
	StopAndDrainTimer(timer)

	// Stop should return false to indicate the timer has already been stopped
	if timer.Stop() {
		t.Fatal("error stopping and draining an already stopped timer. Expected stop() to return false, go true")
	}

	// Check without blocking if timerChan received `true`
	select {
	case <-timerChan:
		t.Fatal("error stopping and draining an already stopped timer")
	default:
		return
	}	
}
