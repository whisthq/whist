// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"). You may
// not use this file except in compliance with the License. A copy of the
// License is located at
//
//	http://aws.amazon.com/apache2.0/
//
// or in the "license" file accompanying this file. This file is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing
// permissions and limitations under the License.

package engine

import (
	"context"
	"io"
	"strings"
	"sync"
	"time"

	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/engine/execcmd"

	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api"
	apicontainer "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api/container"
	apicontainerstatus "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api/container/status"
	apierrors "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api/errors"
	apitask "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api/task"
	apitaskstatus "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api/task/status"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/config"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/credentials"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/dockerclient/dockerapi"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/engine/dependencygraph"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/eventstream"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/statechange"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/taskresource"
	resourcestatus "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/taskresource/status"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/utils/retry"
	utilsync "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/utils/sync"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/utils/ttime"

	"github.com/cihub/seelog"
)

const (
	// waitForPullCredentialsTimeout is the timeout agent trying to wait for pull
	// credentials from acs, after the timeout it will check the credentials manager
	// and start processing the task or start another round of waiting
	waitForPullCredentialsTimeout            = 1 * time.Minute
	defaultTaskSteadyStatePollInterval       = 5 * time.Minute
	defaultTaskSteadyStatePollIntervalJitter = 30 * time.Second
	transitionPollTime                       = 5 * time.Second
	stoppedSentWaitInterval                  = 30 * time.Second
	maxStoppedWaitTimes                      = 72 * time.Hour / stoppedSentWaitInterval
	taskUnableToTransitionToStoppedReason    = "TaskStateError: Agent could not progress task's state to stopped"
)

var (
	_stoppedSentWaitInterval = stoppedSentWaitInterval
	_maxStoppedWaitTimes     = int(maxStoppedWaitTimes)
)

type dockerContainerChange struct {
	container *apicontainer.Container
	event     dockerapi.DockerContainerChangeEvent
}

// resourceStateChange represents the required status change after resource transition
type resourceStateChange struct {
	resource  taskresource.TaskResource
	nextState resourcestatus.ResourceStatus
	err       error
}

type acsTransition struct {
	seqnum        int64
	desiredStatus apitaskstatus.TaskStatus
}

// containerTransition defines the struct for a container to transition
type containerTransition struct {
	nextState      apicontainerstatus.ContainerStatus
	actionRequired bool
	blockedOn      *apicontainer.DependsOn
	reason         error
}

// resourceTransition defines the struct for a resource to transition.
type resourceTransition struct {
	// nextState represents the next known status that the resource can move to
	nextState resourcestatus.ResourceStatus
	// status is the string value of nextState
	status string
	// actionRequired indicates if the transition function needs to be called for
	// the transition to be complete
	actionRequired bool
	// reason represents the error blocks transition
	reason error
}

// managedTask is a type that is meant to manage the lifecycle of a task.
// There should be only one managed task construct for a given task arn and the
// managed task should be the only thing to modify the task's known or desired statuses.
//
// The managedTask should run serially in a single goroutine in which it reads
// messages from the two given channels and acts upon them.
// This design is chosen to allow a safe level if isolation and avoid any race
// conditions around the state of a task.
// The data sources (e.g. docker, acs) that write to the task's channels may
// block and it is expected that the managedTask listen to those channels
// almost constantly.
// The general operation should be:
//  1) Listen to the channels
//  2) On an event, update the status of the task and containers (known/desired)
//  3) Figure out if any action needs to be done. If so, do it
//  4) GOTO 1
// Item '3' obviously might lead to some duration where you are not listening
// to the channels. However, this can be solved by kicking off '3' as a
// goroutine and then only communicating the result back via the channels
// (obviously once you kick off a goroutine you give up the right to write the
// task's statuses yourself)
type managedTask struct {
	*apitask.Task
	ctx    context.Context
	cancel context.CancelFunc

	engine             *DockerTaskEngine
	cfg                *config.Config
	credentialsManager credentials.Manager
	taskStopWG         *utilsync.SequentialWaitGroup

	acsMessages                chan acsTransition
	dockerMessages             chan dockerContainerChange
	resourceStateChangeEvent   chan resourceStateChange
	stateChangeEvents          chan statechange.Event
	containerChangeEventStream *eventstream.EventStream

	// unexpectedStart is a once that controls stopping a container that
	// unexpectedly started one time.
	// This exists because a 'start' after a container is meant to be stopped is
	// possible under some circumstances (e.g. a timeout). However, if it
	// continues to 'start' when we aren't asking it to, let it go through in
	// case it's a user trying to debug it or in case we're fighting with another
	// thing managing the container.
	unexpectedStart sync.Once

	_time     ttime.Time
	_timeOnce sync.Once

	// steadyStatePollInterval is the duration that a managed task waits
	// once the task gets into steady state before polling the state of all of
	// the task's containers to re-evaluate if the task is still in steady state
	// This is set to defaultTaskSteadyStatePollInterval in production code.
	// This can be used by tests that are looking to ensure that the steady state
	// verification logic gets executed to set it to a low interval
	steadyStatePollInterval       time.Duration
	steadyStatePollIntervalJitter time.Duration
}

// newManagedTask is a method on DockerTaskEngine to create a new managedTask.
// This method must only be called when the engine.processTasks write lock is
// already held.
func (engine *DockerTaskEngine) newManagedTask(task *apitask.Task) *managedTask {
	ctx, cancel := context.WithCancel(engine.ctx)
	t := &managedTask{
		ctx:                           ctx,
		cancel:                        cancel,
		Task:                          task,
		acsMessages:                   make(chan acsTransition),
		dockerMessages:                make(chan dockerContainerChange),
		resourceStateChangeEvent:      make(chan resourceStateChange),
		engine:                        engine,
		cfg:                           engine.cfg,
		stateChangeEvents:             engine.stateChangeEvents,
		containerChangeEventStream:    engine.containerChangeEventStream,
		credentialsManager:            engine.credentialsManager,
		taskStopWG:                    engine.taskStopGroup,
		steadyStatePollInterval:       engine.taskSteadyStatePollInterval,
		steadyStatePollIntervalJitter: engine.taskSteadyStatePollIntervalJitter,
	}
	engine.managedTasks[task.Arn] = t
	return t
}

// overseeTask is the main goroutine of the managedTask. It runs an infinite
// loop of receiving messages and attempting to take action based on those
// messages.
func (mtask *managedTask) overseeTask() {
	// Do a single updatestatus at the beginning to create the container
	// `desiredstatus`es which are a construct of the engine used only here,
	// not present on the backend
	mtask.UpdateStatus()
	// If this was a 'state restore', send all unsent statuses
	mtask.emitCurrentStatus()

	// Wait for host resources required by this task to become available
	mtask.waitForHostResources()

	// Main infinite loop. This is where we receive messages and dispatch work.
	for {
		if mtask.shouldExit() {
			return
		}

		// If it's steadyState, just spin until we need to do work
		for mtask.steadyState() {
			mtask.waitSteady()
		}

		if mtask.shouldExit() {
			return
		}

		if !mtask.GetKnownStatus().Terminal() {
			// If we aren't terminal and we aren't steady state, we should be
			// able to move some containers along.
			seelog.Infof("Managed task [%s]: task not steady state or terminal; progressing it",
				mtask.Arn)

			mtask.progressTask()
		}

		if mtask.GetKnownStatus().Terminal() {
			break
		}
	}
	// We only break out of the above if this task is known to be stopped. Do
	// onetime cleanup here, including removing the task after a timeout
	seelog.Infof("Managed task [%s]: task has reached stopped. Waiting for container cleanup", mtask.Arn)
	mtask.cleanupCredentials()
	if mtask.StopSequenceNumber != 0 {
		seelog.Debugf("Managed task [%s]: marking done for this sequence: %d",
			mtask.Arn, mtask.StopSequenceNumber)
		mtask.taskStopWG.Done(mtask.StopSequenceNumber)
	}
	// TODO: make this idempotent on agent restart
	go mtask.releaseIPInIPAM()
	mtask.cleanupTask(mtask.cfg.TaskCleanupWaitDuration)
}

// shouldExit checks if the task manager should exit, as the agent is exiting.
func (mtask *managedTask) shouldExit() bool {
	select {
	case <-mtask.ctx.Done():
		return true
	default:
		return false
	}
}

// emitCurrentStatus emits a container event for every container and a task
// event for the task
func (mtask *managedTask) emitCurrentStatus() {
	for _, container := range mtask.Containers {
		mtask.emitContainerEvent(mtask.Task, container, "")
	}
	mtask.emitTaskEvent(mtask.Task, "")
}

// waitForHostResources waits for host resources to become available to start
// the task. This involves waiting for previous stops to complete so the
// resources become free.
func (mtask *managedTask) waitForHostResources() {
	if mtask.StartSequenceNumber == 0 {
		// This is the first transition on this host. No need to wait
		return
	}
	if mtask.GetDesiredStatus().Terminal() {
		// Task's desired status is STOPPED. No need to wait in this case either
		return
	}

	seelog.Infof("Managed task [%s]: waiting for any previous stops to complete. Sequence number: %d",
		mtask.Arn, mtask.StartSequenceNumber)

	othersStoppedCtx, cancel := context.WithCancel(mtask.ctx)
	defer cancel()

	go func() {
		mtask.taskStopWG.Wait(mtask.StartSequenceNumber)
		cancel()
	}()

	for !mtask.waitEvent(othersStoppedCtx.Done()) {
		if mtask.GetDesiredStatus().Terminal() {
			// If we end up here, that means we received a start then stop for this
			// task before a task that was expected to stop before it could
			// actually stop
			break
		}
	}
	seelog.Infof("Managed task [%s]: wait over; ready to move towards status: %s",
		mtask.Arn, mtask.GetDesiredStatus().String())
}

// waitSteady waits for a task to leave steady-state by waiting for a new
// event, or a timeout.
func (mtask *managedTask) waitSteady() {
	seelog.Infof("Managed task [%s]: task at steady state: %s", mtask.Arn, mtask.GetKnownStatus().String())

	timeoutCtx, cancel := context.WithTimeout(mtask.ctx, retry.AddJitter(mtask.steadyStatePollInterval, mtask.steadyStatePollIntervalJitter))
	defer cancel()
	timedOut := mtask.waitEvent(timeoutCtx.Done())
	if mtask.shouldExit() {
		return
	}

	if timedOut {
		seelog.Infof("Managed task [%s]: checking to verify it's still at steady state.", mtask.Arn)
		go mtask.engine.checkTaskState(mtask.Task)
	}
}

// steadyState returns if the task is in a steady state. Steady state is when task's desired
// and known status are both RUNNING
func (mtask *managedTask) steadyState() bool {
	select {
	case <-mtask.ctx.Done():
		seelog.Infof("Managed task [%s]: agent task manager exiting.", mtask.Arn)
		return false
	default:
		taskKnownStatus := mtask.GetKnownStatus()
		return taskKnownStatus == apitaskstatus.TaskRunning && taskKnownStatus >= mtask.GetDesiredStatus()
	}
}

// cleanupCredentials removes credentials for a stopped task
func (mtask *managedTask) cleanupCredentials() {
	taskCredentialsID := mtask.GetCredentialsID()
	if taskCredentialsID != "" {
		mtask.credentialsManager.RemoveCredentials(taskCredentialsID)
	}
}

// waitEvent waits for any event to occur. If an event occurs, the appropriate
// handler is called. Generally the stopWaiting arg is the context's Done
// channel. When the Done channel is signalled by the context, waitEvent will
// return true.
func (mtask *managedTask) waitEvent(stopWaiting <-chan struct{}) bool {
	seelog.Infof("Managed task [%s]: waiting for event for task", mtask.Arn)
	select {
	case acsTransition := <-mtask.acsMessages:
		seelog.Infof("Managed task [%s]: got acs event", mtask.Arn)
		mtask.handleDesiredStatusChange(acsTransition.desiredStatus, acsTransition.seqnum)
		return false
	case dockerChange := <-mtask.dockerMessages:
		mtask.handleContainerChange(dockerChange)
		return false
	case resChange := <-mtask.resourceStateChangeEvent:
		res := resChange.resource
		seelog.Infof("Managed task [%s]: got resource [%s] event: [%s]",
			mtask.Arn, res.GetName(), res.StatusString(resChange.nextState))
		mtask.handleResourceStateChange(resChange)
		return false
	case <-stopWaiting:
		return true
	}
}

// handleDesiredStatusChange updates the desired status on the task. Updates
// only occur if the new desired status is "compatible" (farther along than the
// current desired state); "redundant" (less-than or equal desired states) are
// ignored and dropped.
func (mtask *managedTask) handleDesiredStatusChange(desiredStatus apitaskstatus.TaskStatus, seqnum int64) {
	// Handle acs message changes this task's desired status to whatever
	// acs says it should be if it is compatible
	seelog.Infof("Managed task [%s]: new acs transition to: %s; sequence number: %d; task stop sequence number: %d",
		mtask.Arn, desiredStatus.String(), seqnum, mtask.StopSequenceNumber)
	if desiredStatus <= mtask.GetDesiredStatus() {
		seelog.Infof("Managed task [%s]: redundant task transition from [%s] to [%s], ignoring",
			mtask.Arn, mtask.GetDesiredStatus().String(), desiredStatus.String())
		return
	}
	if desiredStatus == apitaskstatus.TaskStopped && seqnum != 0 && mtask.GetStopSequenceNumber() == 0 {
		seelog.Infof("Managed task [%s]: task moving to stopped, adding to stopgroup with sequence number: %d",
			mtask.Arn, seqnum)
		mtask.SetStopSequenceNumber(seqnum)
		mtask.taskStopWG.Add(seqnum, 1)
	}
	mtask.SetDesiredStatus(desiredStatus)
	mtask.UpdateDesiredStatus()
	mtask.engine.saveTaskData(mtask.Task)
}

// handleContainerChange updates a container's known status. If the message
// contains any interesting information (like exit codes or ports), they are
// propagated.
func (mtask *managedTask) handleContainerChange(containerChange dockerContainerChange) {
	// locate the container
	container := containerChange.container
	runtimeID := container.GetRuntimeID()
	event := containerChange.event
	seelog.Infof("Managed task [%s]: Container [name=%s runtimeID=%s]: handling container change event [%s]",
		mtask.Arn, container.Name, runtimeID, event.Status.String())
	seelog.Debugf("Managed task [%s]: Container [name=%s runtimeID=%s]: container change event [%s]",
		mtask.Arn, container.Name, runtimeID, event)
	found := mtask.isContainerFound(container)
	if !found {
		seelog.Criticalf("Managed task [%s]: Container [name=%s runtimeID=%s]: state error; invoked with another task's container!",
			mtask.Arn, container.Name, runtimeID)
		return
	}

	// If this is a backwards transition stopped->running, the first time set it
	// to be known running so it will be stopped. Subsequently ignore these backward transitions
	containerKnownStatus := container.GetKnownStatus()
	mtask.handleStoppedToRunningContainerTransition(event.Status, container)
	if event.Status <= containerKnownStatus {
		seelog.Infof("Managed task [%s]: Container [name=%s runtimeID=%s]: container change %s->%s is redundant",
			mtask.Arn, container.Name, runtimeID, containerKnownStatus.String(), event.Status.String())

		// Only update container metadata when status stays RUNNING
		if event.Status == containerKnownStatus && event.Status == apicontainerstatus.ContainerRunning {
			updateContainerMetadata(&event.DockerContainerMetadata, container, mtask.Task)
		}
		return
	}

	// Container has progressed its status if we reach here. Make sure to save it to database.
	defer mtask.engine.saveContainerData(container)

	// Update the container to be known
	currentKnownStatus := containerKnownStatus
	container.SetKnownStatus(event.Status)
	updateContainerMetadata(&event.DockerContainerMetadata, container, mtask.Task)

	if event.Error != nil {
		proceedAnyway := mtask.handleEventError(containerChange, currentKnownStatus)
		if !proceedAnyway {
			return
		}
	}

	if execcmd.IsExecEnabledContainer(container) && container.GetKnownStatus() == apicontainerstatus.ContainerStopped {
		// if this is an execute-command-enabled container STOPPED event, we should emit a corresponding managedAgent event
		mtask.handleManagedAgentStoppedTransition(container, execcmd.ExecuteCommandAgentName)
	}

	mtask.RecordExecutionStoppedAt(container)
	seelog.Debugf("Managed task [%s]: Container [name=%s runtimeID=%s]: sending container change event to tcs, status: %s",
		mtask.Arn, container.Name, runtimeID, event.Status.String())
	err := mtask.containerChangeEventStream.WriteToEventStream(event)
	if err != nil {
		seelog.Warnf("Managed task [%s]: Container [name=%s runtimeID=%s]: failed to write container change event to tcs event stream: %v",
			mtask.Arn, container.Name, runtimeID, err)
	}

	mtask.emitContainerEvent(mtask.Task, container, "")
	if mtask.UpdateStatus() {
		seelog.Infof("Managed task [%s]: Container [name=%s runtimeID=%s]: container change also resulted in task change [%s]",
			mtask.Arn, container.Name, runtimeID, mtask.GetDesiredStatus().String())
		// If knownStatus changed, let it be known
		var taskStateChangeReason string
		if mtask.GetKnownStatus().Terminal() {
			taskStateChangeReason = mtask.Task.GetTerminalReason()
		}
		mtask.emitTaskEvent(mtask.Task, taskStateChangeReason)
		// Save the new task status to database.
		mtask.engine.saveTaskData(mtask.Task)
	}
}

// handleResourceStateChange attempts to update resource's known status depending on
// the current status and errors during transition
func (mtask *managedTask) handleResourceStateChange(resChange resourceStateChange) {
	// locate the resource
	res := resChange.resource
	if !mtask.isResourceFound(res) {
		seelog.Criticalf("Managed task [%s]: state error; invoked with another task's resource [%s]",
			mtask.Arn, res.GetName())
		return
	}

	status := resChange.nextState
	err := resChange.err
	currentKnownStatus := res.GetKnownStatus()

	if status <= currentKnownStatus {
		seelog.Infof("Managed task [%s]: redundant resource state change. %s to %s, but already %s",
			mtask.Arn, res.GetName(), res.StatusString(status), res.StatusString(currentKnownStatus))
		return
	}

	// There is a resource state change. Resource is stored as part of the task, so make sure to save the task
	// at the end.
	defer mtask.engine.saveTaskData(mtask.Task)

	// Set known status regardless of error so the applied status can be cleared. If there is error,
	// the known status might be set again below (but that won't affect the applied status anymore).
	// This follows how container state change is handled.
	res.SetKnownStatus(status)
	if err == nil {
		return
	}

	if status == res.SteadyState() { // Failed to create resource.
		seelog.Errorf("Managed task [%s]: failed to create task resource [%s]: %v", mtask.Arn, res.GetName(), err)
		res.SetKnownStatus(currentKnownStatus) // Set status back to None.

		seelog.Infof("Managed task [%s]: marking task desired status to STOPPED", mtask.Arn)
		mtask.SetDesiredStatus(apitaskstatus.TaskStopped)
		mtask.Task.SetTerminalReason(res.GetTerminalReason())
	}
}

func (mtask *managedTask) emitResourceChange(change resourceStateChange) {
	select {
	case <-mtask.ctx.Done():
		seelog.Infof("Managed task [%s]: unable to emit resource state change due to exit", mtask.Arn)
	case mtask.resourceStateChangeEvent <- change:
	}
}

func (mtask *managedTask) emitTaskEvent(task *apitask.Task, reason string) {
	event, err := api.NewTaskStateChangeEvent(task, reason)
	if err != nil {
		seelog.Debugf("Managed task [%s]: skipping emitting event for task [%s]: %v",
			task.Arn, reason, err)
		return
	}
	seelog.Infof("Managed task [%s]: sending task change event [%s]", mtask.Arn, event.String())
	select {
	case <-mtask.ctx.Done():
		seelog.Infof("Managed task [%s]: unable to send task change event [%s] due to exit", mtask.Arn, event.String())
	case mtask.stateChangeEvents <- event:
	}
	seelog.Infof("Managed task [%s]: sent task change event [%s]", mtask.Arn, event.String())
}

// emitManagedAgentEvent passes a special task event up through the taskEvents channel if there are managed
// agent changes in the container passed as parameter.
// It will omit events the backend would not process
func (mtask *managedTask) emitManagedAgentEvent(task *apitask.Task, cont *apicontainer.Container, managedAgentName string, reason string) {
	event, err := api.NewManagedAgentChangeEvent(task, cont, managedAgentName, reason)
	if err != nil {
		seelog.Errorf("Managed task [%s]: skipping emitting ManagedAgent event for task [%s]: %v",
			task.Arn, reason, err)
		return
	}
	seelog.Infof("Managed task [%s]: sending managed agent event [%s]", mtask.Arn, event.String())
	select {
	case <-mtask.ctx.Done():
		seelog.Infof("Managed task [%s]: unable to send managed agent event [%s] due to exit", mtask.Arn, event.String())
	case mtask.stateChangeEvents <- event:
	}
	seelog.Infof("Managed task [%s]: sent managed agent event [%s]", mtask.Arn, event.String())
}

// emitContainerEvent passes a given event up through the containerEvents channel if necessary.
// It will omit events the backend would not process and will perform best-effort deduplication of events.
func (mtask *managedTask) emitContainerEvent(task *apitask.Task, cont *apicontainer.Container, reason string) {
	event, err := api.NewContainerStateChangeEvent(task, cont, reason)
	if err != nil {
		seelog.Debugf("Managed task [%s]: skipping emitting event for container [%s]: %v",
			task.Arn, cont.Name, err)
		return
	}
	mtask.doEmitContainerEvent(event)
}

func (mtask *managedTask) doEmitContainerEvent(event api.ContainerStateChange) {

	seelog.Infof("Managed task [%s]: Container [%s]: sending container change event: %s",
		mtask.Arn, event.Container.Name, event.String())
	select {
	case <-mtask.ctx.Done():
		seelog.Infof("Managed task [%s]: Container [%s]: unable to send container change event [%s] due to exit",
			mtask.Arn, event.Container.Name, event.String())
	case mtask.stateChangeEvents <- event:
	}
	seelog.Infof("Managed task [%s]: Container [%s]: sent container change event: %s",
		mtask.Arn, event.Container.Name, event.String())

}

func (mtask *managedTask) emitDockerContainerChange(change dockerContainerChange) {
	select {
	case <-mtask.ctx.Done():
		seelog.Infof("Managed task [%s]: unable to emit docker container change due to exit", mtask.Arn)
	case mtask.dockerMessages <- change:
	}
}

func (mtask *managedTask) emitACSTransition(transition acsTransition) {
	select {
	case <-mtask.ctx.Done():
		seelog.Infof("Managed task [%s]: unable to emit docker container change due to exit", mtask.Arn)
	case mtask.acsMessages <- transition:
	}
}

func (mtask *managedTask) isContainerFound(container *apicontainer.Container) bool {
	found := false
	for _, c := range mtask.Containers {
		if container == c {
			found = true
			break
		}
	}
	return found
}

func (mtask *managedTask) isResourceFound(res taskresource.TaskResource) bool {
	for _, r := range mtask.GetResources() {
		if res.GetName() == r.GetName() {
			return true
		}
	}
	return false
}

// handleStoppedToRunningContainerTransition detects a "backwards" container
// transition where a known-stopped container is found to be running again and
// handles it.
func (mtask *managedTask) handleStoppedToRunningContainerTransition(status apicontainerstatus.ContainerStatus, container *apicontainer.Container) {
	containerKnownStatus := container.GetKnownStatus()
	if status > containerKnownStatus {
		// Event status is greater than container's known status.
		// This is not a backward transition, return
		return
	}
	if containerKnownStatus != apicontainerstatus.ContainerStopped {
		// Container's known status is not STOPPED. Nothing to do here.
		return
	}
	if !status.IsRunning() {
		// Container's 'to' transition was not either of RUNNING or RESOURCES_PROVISIONED
		// states. Nothing to do in this case as well
		return
	}
	// If the container becomes running after we've stopped it (possibly
	// because we got an error running it and it ran anyways), the first time
	// update it to 'known running' so that it will be driven back to stopped
	mtask.unexpectedStart.Do(func() {
		seelog.Warnf("Managed task [%s]: stopped container [%s] came back; re-stopping it once",
			mtask.Arn, container.Name)
		go mtask.engine.transitionContainer(mtask.Task, container, apicontainerstatus.ContainerStopped)
		// This will not proceed afterwards because status <= knownstatus below
	})
}

// handleManagedAgentStoppedTransition handles a container change event which has a managed agent status
// we should emit ManagedAgent events for certain container events.
func (mtask *managedTask) handleManagedAgentStoppedTransition(container *apicontainer.Container, managedAgentName string) {
	//for now we only have the ExecuteCommandAgent
	switch managedAgentName {
	case execcmd.ExecuteCommandAgentName:
		if !container.UpdateManagedAgentStatus(managedAgentName, apicontainerstatus.ManagedAgentStopped) {
			seelog.Warnf("Managed task [%s]: cannot find managed agent %s for container %s", mtask.Arn, managedAgentName, container.Name)
		}
		mtask.emitManagedAgentEvent(mtask.Task, container, managedAgentName, "Received Container Stopped event")
	default:
		seelog.Warnf("Managed task [%s]: unexpected managed agent [%s] in container [%s]; unable to process stopped container transition event",
			mtask.Arn, managedAgentName, container.Name)
	}
}

// handleEventError handles a container change event error and decides whether
// we should proceed to transition the container
func (mtask *managedTask) handleEventError(containerChange dockerContainerChange, currentKnownStatus apicontainerstatus.ContainerStatus) bool {
	container := containerChange.container
	event := containerChange.event
	if container.ApplyingError == nil {
		container.ApplyingError = apierrors.NewNamedError(event.Error)
	}
	switch event.Status {
	// event.Status is the desired container transition from container's known status
	// (* -> event.Status)
	case apicontainerstatus.ContainerPulled:
		// If the agent pull behavior is always or once, we receive the error because
		// the image pull fails, the task should fail. If we don't fail task here,
		// then the cached image will probably be used for creating container, and we
		// don't want to use cached image for both cases.
		if mtask.cfg.ImagePullBehavior == config.ImagePullAlwaysBehavior ||
			mtask.cfg.ImagePullBehavior == config.ImagePullOnceBehavior {
			seelog.Errorf("Managed task [%s]: error while pulling image %s for container %s , moving task to STOPPED: %v",
				mtask.Arn, container.Image, container.Name, event.Error)
			// The task should be stopped regardless of whether this container is
			// essential or non-essential.
			mtask.SetDesiredStatus(apitaskstatus.TaskStopped)
			return false
		}
		// If the agent pull behavior is prefer-cached, we receive the error because
		// the image pull fails and there is no cached image in local, we don't make
		// the task fail here, will let create container handle it instead.
		// If the agent pull behavior is default, use local image cache directly,
		// assuming it exists.
		seelog.Errorf("Managed task [%s]: error while pulling container %s and image %s, will try to run anyway: %v",
			mtask.Arn, container.Name, container.Image, event.Error)
		// proceed anyway
		return true
	case apicontainerstatus.ContainerStopped:
		// Container's desired transition was to 'STOPPED'
		return mtask.handleContainerStoppedTransitionError(event, container, currentKnownStatus)
	case apicontainerstatus.ContainerStatusNone:
		fallthrough
	case apicontainerstatus.ContainerCreated:
		// No need to explicitly stop containers if this is a * -> NONE/CREATED transition
		seelog.Errorf("Managed task [%s]: error creating container [%s]; marking its desired status as STOPPED: %v",
			mtask.Arn, container.Name, event.Error)
		container.SetKnownStatus(currentKnownStatus)
		container.SetDesiredStatus(apicontainerstatus.ContainerStopped)
		return false
	default:
		// If this is a * -> RUNNING / RESOURCES_PROVISIONED transition, we need to stop
		// the container.
		seelog.Errorf("Managed task [%s]: error starting/provisioning container[%s (Runtime ID: %s)]; marking its desired status as STOPPED: %v",
			mtask.Arn, container.Name, container.GetRuntimeID(), event.Error)
		container.SetKnownStatus(currentKnownStatus)
		container.SetDesiredStatus(apicontainerstatus.ContainerStopped)
		errorName := event.Error.ErrorName()
		errorStr := event.Error.Error()
		shouldForceStop := false
		if errorName == dockerapi.DockerTimeoutErrorName || errorName == dockerapi.CannotInspectContainerErrorName {
			// If there's an error with inspecting the container or in case of timeout error,
			// we'll assume that the container has transitioned to RUNNING and issue
			// a stop. See #1043 for details
			shouldForceStop = true
		} else if errorName == dockerapi.CannotStartContainerErrorName && strings.HasSuffix(errorStr, io.EOF.Error()) {
			// If we get an EOF error from Docker when starting the container, we don't really know whether the
			// container is started anyway. So issuing a stop here as well. See #1708.
			shouldForceStop = true
		}

		if shouldForceStop {
			seelog.Warnf("Managed task [%s]: forcing container [%s (Runtime ID: %s)] to stop",
				mtask.Arn, container.Name, container.GetRuntimeID())
			go mtask.engine.transitionContainer(mtask.Task, container, apicontainerstatus.ContainerStopped)
		}
		// Container known status not changed, no need for further processing
		return false
	}
}

// handleContainerStoppedTransitionError handles an error when transitioning a container to
// STOPPED. It returns a boolean indicating whether the tak can continue with updating its
// state
func (mtask *managedTask) handleContainerStoppedTransitionError(event dockerapi.DockerContainerChangeEvent,
	container *apicontainer.Container,
	currentKnownStatus apicontainerstatus.ContainerStatus) bool {
	// If we were trying to transition to stopped and had a timeout error
	// from docker, reset the known status to the current status and return
	// This ensures that we don't emit a containerstopped event; a
	// terminal container event from docker event stream will instead be
	// responsible for the transition. Alternatively, the steadyState check
	// could also trigger the progress and have another go at stopping the
	// container
	if event.Error.ErrorName() == dockerapi.DockerTimeoutErrorName {
		seelog.Infof("Managed task [%s]: '%s' error stopping container [%s (Runtime ID: %s)]. Ignoring state change: %v",
			mtask.Arn, dockerapi.DockerTimeoutErrorName, container.Name, container.GetRuntimeID(), event.Error.Error())
		container.SetKnownStatus(currentKnownStatus)
		return false
	}
	// If docker returned a transient error while trying to stop a container,
	// reset the known status to the current status and return
	cannotStopContainerError, ok := event.Error.(cannotStopContainerError)
	if ok && cannotStopContainerError.IsRetriableError() {
		seelog.Infof("Managed task [%s]: error stopping the container [%s (Runtime ID: %s)]. Ignoring state change: %v",
			mtask.Arn, container.Name, container.GetRuntimeID(), cannotStopContainerError.Error())
		container.SetKnownStatus(currentKnownStatus)
		return false
	}

	// If we were trying to transition to stopped and had an error, we
	// clearly can't just continue trying to transition it to stopped
	// again and again. In this case, assume it's stopped (or close
	// enough) and get on with it
	// This can happen in cases where the container we tried to stop
	// was already stopped or did not exist at all.
	seelog.Warnf("Managed task [%s]: 'docker stop' for container [%s] returned %s: %s",
		mtask.Arn, container.Name, event.Error.ErrorName(), event.Error.Error())
	container.SetKnownStatus(apicontainerstatus.ContainerStopped)
	container.SetDesiredStatus(apicontainerstatus.ContainerStopped)
	return true
}

// progressTask tries to step forwards all containers and resources that are able to be
// transitioned in the task's current state.
// It will continue listening to events from all channels while it does so, but
// none of those changes will be acted upon until this set of requests to
// docker completes.
// Container changes may also prompt the task status to change as well.
func (mtask *managedTask) progressTask() {
	seelog.Debugf("Managed task [%s]: progressing containers and resources in task", mtask.Arn)
	// max number of transitions length to ensure writes will never block on
	// these and if we exit early transitions can exit the goroutine and it'll
	// get GC'd eventually
	resources := mtask.GetResources()
	transitionChange := make(chan struct{}, len(mtask.Containers)+len(resources))
	transitionChangeEntity := make(chan string, len(mtask.Containers)+len(resources))

	// startResourceTransitions should always be called before startContainerTransitions,
	// else it might result in a state where none of the containers can transition and
	// task may be moved to stopped.
	// anyResourceTransition is set to true when transition function needs to be called or
	// known status can be changed
	anyResourceTransition, resTransitions := mtask.startResourceTransitions(
		func(resource taskresource.TaskResource, nextStatus resourcestatus.ResourceStatus) {
			mtask.transitionResource(resource, nextStatus)
			transitionChange <- struct{}{}
			transitionChangeEntity <- resource.GetName()
		})

	anyContainerTransition, blockedDependencies, contTransitions, reasons := mtask.startContainerTransitions(
		func(container *apicontainer.Container, nextStatus apicontainerstatus.ContainerStatus) {
			mtask.engine.transitionContainer(mtask.Task, container, nextStatus)
			transitionChange <- struct{}{}
			transitionChangeEntity <- container.Name
		})

	atLeastOneTransitionStarted := anyResourceTransition || anyContainerTransition

	blockedByOrderingDependencies := len(blockedDependencies) > 0

	// If no transitions happened and we aren't blocked by ordering dependencies, then we are possibly in a state where
	// its impossible for containers to move forward. We will do an additional check to see if we are waiting for ACS
	// execution credentials. If not, then we will abort the task progression.
	if !atLeastOneTransitionStarted && !blockedByOrderingDependencies {
		if !mtask.isWaitingForACSExecutionCredentials(reasons) {
			mtask.handleContainersUnableToTransitionState()
		}
		return
	}

	// If no containers are starting and we are blocked on ordering dependencies, we should watch for the task to change
	// over time. This will update the containers if they become healthy or stop, which makes it possible for the
	// conditions "HEALTHY" and "SUCCESS" to succeed.
	if !atLeastOneTransitionStarted && blockedByOrderingDependencies {
		go mtask.engine.checkTaskState(mtask.Task)
		ctx, cancel := context.WithTimeout(context.Background(), transitionPollTime)
		defer cancel()
		for timeout := mtask.waitEvent(ctx.Done()); !timeout; {
			timeout = mtask.waitEvent(ctx.Done())
		}
		return
	}

	// combine the resource and container transitions
	transitions := make(map[string]string)
	for k, v := range resTransitions {
		transitions[k] = v
	}
	for k, v := range contTransitions {
		transitions[k] = v.String()
	}

	// We've kicked off one or more transitions, wait for them to
	// complete, but keep reading events as we do. in fact, we have to for
	// transitions to complete
	mtask.waitForTransition(transitions, transitionChange, transitionChangeEntity)
	// update the task status
	if mtask.UpdateStatus() {
		seelog.Infof("Managed task [%s]: container or resource change also resulted in task change", mtask.Arn)

		// If knownStatus changed, let it be known
		var taskStateChangeReason string
		if mtask.GetKnownStatus().Terminal() {
			taskStateChangeReason = mtask.Task.GetTerminalReason()
		}
		mtask.emitTaskEvent(mtask.Task, taskStateChangeReason)
	}
}

// isWaitingForACSExecutionCredentials checks if the container that can't be transitioned
// was caused by waiting for credentials and start waiting
func (mtask *managedTask) isWaitingForACSExecutionCredentials(reasons []error) bool {
	for _, reason := range reasons {
		if reason == dependencygraph.CredentialsNotResolvedErr {
			seelog.Infof("Managed task [%s]: waiting for credentials to pull from ECR", mtask.Arn)

			timeoutCtx, timeoutCancel := context.WithTimeout(mtask.ctx, waitForPullCredentialsTimeout)
			defer timeoutCancel()

			timedOut := mtask.waitEvent(timeoutCtx.Done())
			if timedOut {
				seelog.Infof("Managed task [%s]: timed out waiting for acs credentials message", mtask.Arn)
			}
			return true
		}
	}
	return false
}

// startContainerTransitions steps through each container in the task and calls
// the passed transition function when a transition should occur.
func (mtask *managedTask) startContainerTransitions(transitionFunc containerTransitionFunc) (bool, map[string]apicontainer.DependsOn, map[string]apicontainerstatus.ContainerStatus, []error) {
	anyCanTransition := false
	var reasons []error
	blocked := make(map[string]apicontainer.DependsOn)
	transitions := make(map[string]apicontainerstatus.ContainerStatus)
	for _, cont := range mtask.Containers {
		transition := mtask.containerNextState(cont)
		if transition.reason != nil {
			// container can't be transitioned
			reasons = append(reasons, transition.reason)
			if transition.blockedOn != nil {
				blocked[cont.Name] = *transition.blockedOn
			}
			continue
		}

		// If the container is already in a transition, skip
		if transition.actionRequired && !cont.SetAppliedStatus(transition.nextState) {
			// At least one container is able to be moved forwards, so we're not deadlocked
			anyCanTransition = true
			continue
		}

		// At least one container is able to be moved forwards, so we're not deadlocked
		anyCanTransition = true

		if !transition.actionRequired {
			// Updating the container status without calling any docker API, send in
			// a goroutine so that it won't block here before the waitForContainerTransition
			// was called after this function. And all the events sent to mtask.dockerMessages
			// will be handled by handleContainerChange.
			go func(cont *apicontainer.Container, status apicontainerstatus.ContainerStatus) {
				mtask.dockerMessages <- dockerContainerChange{
					container: cont,
					event: dockerapi.DockerContainerChangeEvent{
						Status: status,
					},
				}
			}(cont, transition.nextState)
			continue
		}
		transitions[cont.Name] = transition.nextState
		go transitionFunc(cont, transition.nextState)
	}

	return anyCanTransition, blocked, transitions, reasons
}

// startResourceTransitions steps through each resource in the task and calls
// the passed transition function when a transition should occur
func (mtask *managedTask) startResourceTransitions(transitionFunc resourceTransitionFunc) (bool, map[string]string) {
	anyCanTransition := false
	transitions := make(map[string]string)
	for _, res := range mtask.GetResources() {
		knownStatus := res.GetKnownStatus()
		desiredStatus := res.GetDesiredStatus()
		if knownStatus >= desiredStatus {
			seelog.Debugf("Managed task [%s]: resource [%s] has already transitioned to or beyond the desired status %s; current known is %s",
				mtask.Arn, res.GetName(), res.StatusString(desiredStatus), res.StatusString(knownStatus))
			continue
		}
		anyCanTransition = true
		transition := mtask.resourceNextState(res)
		// If the resource is already in a transition, skip
		if transition.actionRequired && !res.SetAppliedStatus(transition.nextState) {
			// At least one resource is able to be moved forwards, so we're not deadlocked
			continue
		}
		if !transition.actionRequired {
			// no action is required for the transition, just set the known status without
			// calling any transition function
			go mtask.emitResourceChange(resourceStateChange{
				resource:  res,
				nextState: transition.nextState,
				err:       nil,
			})
			continue
		}
		// At least one resource is able to be move forwards, so we're not deadlocked
		transitions[res.GetName()] = transition.status
		go transitionFunc(res, transition.nextState)
	}
	return anyCanTransition, transitions
}

// transitionResource calls applyResourceState, and then notifies the managed
// task of the change. transitionResource is called by progressTask
func (mtask *managedTask) transitionResource(resource taskresource.TaskResource,
	to resourcestatus.ResourceStatus) {
	err := mtask.applyResourceState(resource, to)

	if mtask.engine.isTaskManaged(mtask.Arn) {
		mtask.emitResourceChange(resourceStateChange{
			resource:  resource,
			nextState: to,
			err:       err,
		})
	}
}

// applyResourceState moves the resource to the given state by calling the
// function defined in the transitionFunctionMap for the state
func (mtask *managedTask) applyResourceState(resource taskresource.TaskResource,
	nextState resourcestatus.ResourceStatus) error {
	resName := resource.GetName()
	resStatus := resource.StatusString(nextState)
	err := resource.ApplyTransition(nextState)
	if err != nil {
		seelog.Infof("Managed task [%s]: error transitioning resource [%s] to [%s]: %v",
			mtask.Arn, resName, resStatus, err)
		return err
	}
	seelog.Infof("Managed task [%s]: transitioned resource [%s] to [%s]",
		mtask.Arn, resName, resStatus)
	return nil
}

type containerTransitionFunc func(container *apicontainer.Container, nextStatus apicontainerstatus.ContainerStatus)

type resourceTransitionFunc func(resource taskresource.TaskResource, nextStatus resourcestatus.ResourceStatus)

// containerNextState determines the next state a container should go to.
// It returns a transition struct including the information:
// * container state it should transition to,
// * a bool indicating whether any action is required
// * an error indicating why this transition can't happen
//
// 'Stopped, false, ""' -> "You can move it to known stopped, but you don't have to call a transition function"
// 'Running, true, ""' -> "You can move it to running and you need to call the transition function"
// 'None, false, containerDependencyNotResolved' -> "This should not be moved; it has unresolved dependencies;"
//
// Next status is determined by whether the known and desired statuses are
// equal, the next numerically greater (iota-wise) status, and whether
// dependencies are fully resolved.
func (mtask *managedTask) containerNextState(container *apicontainer.Container) *containerTransition {
	containerKnownStatus := container.GetKnownStatus()
	containerDesiredStatus := container.GetDesiredStatus()

	if containerKnownStatus == containerDesiredStatus {
		seelog.Debugf("Managed task [%s]: container [%s (Runtime ID: %s)] at desired status: %s",
			mtask.Arn, container.Name, container.GetRuntimeID(), containerDesiredStatus.String())
		return &containerTransition{
			nextState:      apicontainerstatus.ContainerStatusNone,
			actionRequired: false,
			reason:         dependencygraph.ContainerPastDesiredStatusErr,
		}
	}

	if containerKnownStatus > containerDesiredStatus {
		seelog.Debugf("Managed task [%s]: container [%s (Runtime ID: %s)] has already transitioned beyond desired status(%s): %s",
			mtask.Arn, container.Name, container.GetRuntimeID(), containerKnownStatus.String(), containerDesiredStatus.String())
		return &containerTransition{
			nextState:      apicontainerstatus.ContainerStatusNone,
			actionRequired: false,
			reason:         dependencygraph.ContainerPastDesiredStatusErr,
		}
	}
	if blocked, err := dependencygraph.DependenciesAreResolved(container, mtask.Containers,
		mtask.Task.GetExecutionCredentialsID(), mtask.credentialsManager, mtask.GetResources(), mtask.cfg); err != nil {
		seelog.Debugf("Managed task [%s]: can't apply state to container [%s (Runtime ID: %s)] yet due to unresolved dependencies: %v",
			mtask.Arn, container.Name, container.GetRuntimeID(), err)
		return &containerTransition{
			nextState:      apicontainerstatus.ContainerStatusNone,
			actionRequired: false,
			reason:         err,
			blockedOn:      blocked,
		}
	}

	var nextState apicontainerstatus.ContainerStatus
	if container.DesiredTerminal() {
		nextState = apicontainerstatus.ContainerStopped
		// It's not enough to just check if container is in steady state here
		// we should really check if >= RUNNING <= STOPPED
		if !container.IsRunning() {
			// If the container's AppliedStatus is running, it means the StartContainer
			// api call has already been scheduled, we should not mark it as stopped
			// directly, because when the stopped container comes back, we will end up
			// with either:
			// 1. The task is not cleaned up, the handleStoppedToRunningContainerTransition
			// function will handle this case, but only once. If there are some
			// other stopped containers come back, they will not be stopped by
			// Agent.
			// 2. The task has already been cleaned up, in this case any stopped container
			// will not be stopped by Agent when they come back.
			if container.GetAppliedStatus() == apicontainerstatus.ContainerRunning {
				nextState = apicontainerstatus.ContainerStatusNone
			}

			return &containerTransition{
				nextState:      nextState,
				actionRequired: false,
			}
		}
	} else {
		nextState = container.GetNextKnownStateProgression()
	}
	return &containerTransition{
		nextState:      nextState,
		actionRequired: true,
	}
}

// resourceNextState determines the next state a resource should go to.
// It returns a transition struct including the information:
// * resource state it should transition to,
// * string presentation of the resource state
// * a bool indicating whether any action is required
// * an error indicating why this transition can't happen
//
// Next status is determined by whether the known and desired statuses are
// equal, the next numerically greater (iota-wise) status, and whether
// dependencies are fully resolved.
func (mtask *managedTask) resourceNextState(resource taskresource.TaskResource) *resourceTransition {
	resKnownStatus := resource.GetKnownStatus()
	resDesiredStatus := resource.GetDesiredStatus()

	if resKnownStatus >= resDesiredStatus {
		seelog.Debugf("Managed task [%s]: task resource [%s] has already transitioned to or beyond desired status(%s): %s",
			mtask.Arn, resource.GetName(), resource.StatusString(resDesiredStatus), resource.StatusString(resKnownStatus))
		return &resourceTransition{
			nextState:      resourcestatus.ResourceStatusNone,
			status:         resource.StatusString(resourcestatus.ResourceStatusNone),
			actionRequired: false,
			reason:         dependencygraph.ResourcePastDesiredStatusErr,
		}
	}
	if err := dependencygraph.TaskResourceDependenciesAreResolved(resource, mtask.Containers); err != nil {
		seelog.Debugf("Managed task [%s]: can't apply state to resource [%s] yet due to unresolved dependencies: %v",
			mtask.Arn, resource.GetName(), err)
		return &resourceTransition{
			nextState:      resourcestatus.ResourceStatusNone,
			status:         resource.StatusString(resourcestatus.ResourceStatusNone),
			actionRequired: false,
			reason:         err,
		}
	}

	var nextState resourcestatus.ResourceStatus
	if resource.DesiredTerminal() {
		nextState := resource.TerminalStatus()
		return &resourceTransition{
			nextState:      nextState,
			status:         resource.StatusString(nextState),
			actionRequired: false, // Resource cleanup is done while cleaning up task, so not doing anything here.
		}
	}
	nextState = resource.NextKnownState()
	return &resourceTransition{
		nextState:      nextState,
		status:         resource.StatusString(nextState),
		actionRequired: true,
	}
}

func (mtask *managedTask) handleContainersUnableToTransitionState() {
	seelog.Criticalf("Managed task [%s]: task in a bad state; it's not steadystate but no containers want to transition",
		mtask.Arn)
	if mtask.GetDesiredStatus().Terminal() {
		// Ack, really bad. We want it to stop but the containers don't think
		// that's possible. let's just break out and hope for the best!
		seelog.Criticalf("Managed task [%s]: The state is so bad that we're just giving up on it", mtask.Arn)
		mtask.SetKnownStatus(apitaskstatus.TaskStopped)
		mtask.emitTaskEvent(mtask.Task, taskUnableToTransitionToStoppedReason)
		// TODO we should probably panic here
	} else {
		seelog.Criticalf("Managed task [%s]: moving task to stopped due to bad state", mtask.Arn)
		mtask.handleDesiredStatusChange(apitaskstatus.TaskStopped, 0)
	}
}

func (mtask *managedTask) waitForTransition(transitions map[string]string,
	transition <-chan struct{},
	transitionChangeEntity <-chan string) {
	// There could be multiple transitions, but we just need to wait for one of them
	// to ensure that there is at least one container or resource can be processed in the next
	// progressTask call. This is done by waiting for one transition/acs/docker message.
	if !mtask.waitEvent(transition) {
		seelog.Debugf("Managed task [%s]: received non-transition events", mtask.Arn)
		return
	}
	transitionedEntity := <-transitionChangeEntity
	seelog.Debugf("Managed task [%s]: transition for [%s] finished",
		mtask.Arn, transitionedEntity)
	delete(transitions, transitionedEntity)
	seelog.Debugf("Managed task [%s]: still waiting for: %v", mtask.Arn, transitions)
}

func (mtask *managedTask) time() ttime.Time {
	mtask._timeOnce.Do(func() {
		if mtask._time == nil {
			mtask._time = &ttime.DefaultTime{}
		}
	})
	return mtask._time
}

func (mtask *managedTask) cleanupTask(taskStoppedDuration time.Duration) {
	cleanupTimeDuration := mtask.GetKnownStatusTime().Add(taskStoppedDuration).Sub(ttime.Now())
	cleanupTime := make(<-chan time.Time)
	if cleanupTimeDuration < 0 {
		seelog.Infof("Managed task [%s]: Cleanup Duration has been exceeded. Starting cleanup now ", mtask.Arn)
		cleanupTime = mtask.time().After(time.Nanosecond)
	} else {
		cleanupTime = mtask.time().After(cleanupTimeDuration)
	}
	cleanupTimeBool := make(chan struct{})
	go func() {
		<-cleanupTime
		close(cleanupTimeBool)
	}()
	// wait for the cleanup time to elapse, signalled by cleanupTimeBool
	for !mtask.waitEvent(cleanupTimeBool) {
	}

	// wait for apitaskstatus.TaskStopped to be sent
	ok := mtask.waitForStopReported()
	if !ok {
		seelog.Errorf("Managed task [%s]: aborting cleanup for task as it is not reported as stopped. SentStatus: %s",
			mtask.Arn, mtask.GetSentStatus().String())
		return
	}

	seelog.Infof("Managed task [%s]: cleaning up task's containers and data", mtask.Arn)

	// For the duration of this, simply discard any task events; this ensures the
	// speedy processing of other events for other tasks
	// discard events while the task is being removed from engine state
	go mtask.discardEvents()
	mtask.engine.sweepTask(mtask.Task)
	mtask.engine.deleteTask(mtask.Task)

	// The last thing to do here is to cancel the context, which should cancel
	// all outstanding go routines associated with this managed task.
	mtask.cancel()
}

func (mtask *managedTask) discardEvents() {
	for {
		select {
		case <-mtask.dockerMessages:
		case <-mtask.acsMessages:
		case <-mtask.resourceStateChangeEvent:
		case <-mtask.ctx.Done():
			return
		}
	}
}

// waitForStopReported will wait for the task to be reported stopped and return true, or will time-out and return false.
// Messages on the mtask.dockerMessages and mtask.acsMessages channels will be handled while this function is waiting.
func (mtask *managedTask) waitForStopReported() bool {
	stoppedSentBool := make(chan struct{})
	taskStopped := false
	go func() {
		for i := 0; i < _maxStoppedWaitTimes; i++ {
			// ensure that we block until apitaskstatus.TaskStopped is actually sent
			sentStatus := mtask.GetSentStatus()
			if sentStatus >= apitaskstatus.TaskStopped {
				taskStopped = true
				break
			}
			seelog.Warnf("Managed task [%s]: blocking cleanup until the task has been reported stopped. SentStatus: %s (%d/%d)",
				mtask.Arn, sentStatus.String(), i+1, _maxStoppedWaitTimes)
			mtask._time.Sleep(_stoppedSentWaitInterval)
		}
		stoppedSentBool <- struct{}{}
		close(stoppedSentBool)
	}()
	// wait for apitaskstatus.TaskStopped to be sent
	for !mtask.waitEvent(stoppedSentBool) {
	}
	return taskStopped
}
