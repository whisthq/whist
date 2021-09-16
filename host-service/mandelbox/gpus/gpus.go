/*
Package gpus provides the code to optimally allocate mandelboxes to GPUs.
*/
package gpus // import "github.com/fractal/fractal/host-service/mandelbox/gpus"

import (
	"math"
	"sync"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/metrics"
	"github.com/fractal/fractal/host-service/mandelbox/types"
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
	gpuSlice = make([]GPU, numGPUs)
}

// Index is essentially a handle to a GPU on the system. It is the index of a
// particular GPU in the output of `nvidia-smi`.
type Index int

// MaxMandelboxesPerGPU represents the maximum of mandelboxes we can assign to
// each GPU and still maintain acceptable performance. Note that we are
// assuming that all GPUs on a mandelbox are uniform (and that we are using
// only one instance type).
const MaxMandelboxesPerGPU = 2

var gpuSlice []GPU

type GPU struct {
	isFull bool
	assignedMandelboxes []string
	usage uint8
}

// Lock to protect `gpuSlice`.
var usageLock = new(sync.Mutex)

// Allocate allocates a GPU with room for a mandelbox and returns its index.
func Allocate(dockerID types.DockerID) (Index, error) {
	usageLock.Lock()
	defer usageLock.Unlock()

	// We try to allocate as evenly as possible.
	var minIndex = Index(-1)
	var minVal uint8 = math.MaxUint8
	for i, v := range gpuSlice {
		// Just skip full GPUs.
		if v.isFull {
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
	gpu := &gpuSlice[minIndex]
	dockerId := string(dockerID)
	gpu.usage++
	gpu.assignedMandelboxes = append(gpu.assignedMandelboxes, dockerId) //Store the id of assigned mandelbox

	if gpu.usage == MaxMandelboxesPerGPU {
		gpu.isFull = true
	}

	return minIndex, nil
}

// Free decrements the number of mandelboxes we consider assigned to a given
// GPU indexed by `index`. The Mandelbox id is used to determine if the mandelbox
// was assigned to a given GPU.
func Free(index Index, dockerID types.DockerID) error {
	usageLock.Lock()
	defer usageLock.Unlock()
	gpu := &gpuSlice[index]
	dockerId := string(dockerID)

	if gpu.usage <= 0 {
		return utils.MakeError("Free called on a GPU Index that has no mandelboxes allocated!")
	}
	if !utils.SliceContainsString(gpu.assignedMandelboxes, dockerId) {
		return utils.MakeError("Mandelbox is not allocated on GPU Index!")
	}

	gpu.usage--
	gpu.isFull = false
	gpu.assignedMandelboxes = utils.SliceRemoveString(gpu.assignedMandelboxes, dockerId)
	return nil
}
