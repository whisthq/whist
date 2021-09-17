/*
Package gpus provides the code to optimally allocate mandelboxes to GPUs.
*/
package gpus // import "github.com/fractal/fractal/host-service/mandelbox/gpus"

import (
	"math"
	"sync"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/metrics"
	"github.com/fractal/fractal/host-service/utils"
)

func init() {
	// Initialize usagePerGPU
	metrics, errs := metrics.GetLatest()
	if len(errs) != 0 {
		// We can do a "real" panic in an init function.
		logger.Panicf(nil, "Couldn't initialize GPU allocator, since error getting metrics: %+v", errs)
	}
	numGPUs := metrics.NumberOfGPUs
	gpuMetadata = make([]GPU, numGPUs)
}

// Index is essentially a handle to a GPU on the system. It is the index of a
// particular GPU in the output of `nvidia-smi`.
type Index int

// MaxMandelboxesPerGPU represents the maximum of mandelboxes we can assign to
// each GPU and still maintain acceptable performance. Note that we are
// assuming that all GPUs on a mandelbox are uniform (and that we are using
// only one instance type).
const MaxMandelboxesPerGPU = 2

// gpuMetadata is a slice of GPU structs to keep track of usage and assigned mandelboxes
var gpuMetadata []GPU

// GPU holds the state for any given GPU. It contains an array of mandelbox ids,
// which is used to ensure we only free assigned resources.
type GPU struct {
	assignedMandelboxes []types.MandelboxID
	usage               uint8
}

// Lock to protect `gpuMetadata`.
var usageLock = new(sync.Mutex)

// Allocate allocates a GPU with room for a mandelbox and returns its index.
func Allocate(mandelboxID types.MandelboxID) (Index, error) {
	usageLock.Lock()
	defer usageLock.Unlock()

	// We try to allocate as evenly as possible.
	var minIndex = Index(-1)
	var minVal uint8 = math.MaxUint8
	for i, v := range gpuMetadata {
		// Just skip full GPUs.
		if v.usage >= MaxMandelboxesPerGPU {
			continue
		}
		if v.usage < minVal {
			minVal = v.usage
			minIndex = Index(i)
		}
	}

	if minIndex == Index(-1) || minVal == math.MaxUint8 {
		return -1, utils.MakeError("No free GPUs!")
	}

	// Allocate slot on GPU and return Index
	gpu := &gpuMetadata[minIndex]
	gpu.usage++
	gpu.assignedMandelboxes = append(gpu.assignedMandelboxes, mandelboxID) //Store the id of assigned mandelbox

	return minIndex, nil
}

// Free decrements the number of mandelboxes we consider assigned to a given
// GPU indexed by `index`. The Mandelbox id is used to determine if the mandelbox
// was assigned to a given GPU.
func Free(index Index, mandelboxID types.MandelboxID) error {
	usageLock.Lock()
	defer usageLock.Unlock()
	gpu := &gpuMetadata[index]

	// Copy elements to an interface slice, this is necessary to use the
	// SliceContains and SliceRemove functions.
	assigned := make([]interface{}, len(gpu.assignedMandelboxes))
	for i, v := range gpu.assignedMandelboxes {
		assigned[i] = v
	}

	if gpu.usage <= 0 {
		return utils.MakeError("Free called on a GPU Index that has no mandelboxes allocated!")
	}
	if !utils.SliceContains(assigned, mandelboxID) {
		return utils.MakeError("Mandelbox is not allocated on GPU Index!")
	}

	gpu.usage--

	// Copy resulting elements into the assigned Mandelboxes slice, this is
	// necessary because we need to assert the type to MandelboxID
	updated := utils.SliceRemove(assigned, mandelboxID)
	gpu.assignedMandelboxes = make([]types.MandelboxID, len(updated)) // Replace old slice with empty slice

	for i, v := range updated {
		gpu.assignedMandelboxes[i] = v.(types.MandelboxID)
	}
	return nil
}
