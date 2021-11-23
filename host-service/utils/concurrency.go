package utils

import "time"

// StopAndDrainTimer stops and drains a time.Timer object. We need this
// function so that we can stop a timer without potentially leaking a
// channel/goroutine. Note that this code is kind of subtle (see
// https://github.com/golang/go/issues/27169#issue-353270716).  A previous
// iteration of it had a rare deadlock!
func StopAndDrainTimer(t *time.Timer) {
	if !t.Stop() {
		select {
		case <-t.C:
		default:
		}
	}
}
