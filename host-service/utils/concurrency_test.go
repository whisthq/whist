package utils

import (
	"time"
	"testing"
)

// TestStopAndDrainTimer will confirm no deadlocks occurred and timerChan is not empty
func TestStopAndDrainTimer(t *testing.T) {
	// Simulate the actual usage of this function
	timerChan := make(chan interface{})
	sleepTime := 1
	timer := time.AfterFunc(time.Duration(sleepTime)*time.Millisecond, func() { timerChan <- struct{}{} })

	// Gives timerChan to be filled
	time.Sleep(10000)

	// Does nothing as timer has been stopped
	StopAndDrainTimer(timer)

	select {
	case <-timerChan:
		return
	default:
		t.Fatal("error stopping and draining timer")
	}
}

// TestStopAndDrainTimerAlreadyStopped will confirm no deadlocks occurred and timerChan is empty
func TestStopAndDrainTimerAlreadyStopped(t *testing.T) {
	// Simulate the actual usage of this function
	timerChan := make(chan interface{})
	sleepTime := 60000
	timer := time.AfterFunc(time.Duration(sleepTime)*time.Millisecond, func() { timerChan <- struct{}{} })

	//  Will stop timer
	StopAndDrainTimer(timer)

	select {
	case <-timerChan:
		t.Fatal("error stopping and draining timer when it is already stopped")
	default:
		return
	}
}
