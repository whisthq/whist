package gpus // import "github.com/fractal/fractal/ecs-host-service/mandelbox/gpus"

import (
	"math"
	"sync"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/metrics"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

func init() {
	// Initialize usagePerGPU
	metrics, errs := metrics.GetLatest()
	if len(errs) != 0 {
		// We can do a "real" panic in an init function.
		logger.Panicf(nil, "Couldn't initialize GPU allocator, since error getting metrics: %+v", errs)
	}
	numGPUs := metrics.NumberOfGPUs
	usagePerGPU = make([]uint8, numGPUs)
}

// Index is essentially a handle to a GPU on the system. It is the index of a
// particular GPU in the output of `nvidia-smi`.
type Index int

// MaxMandelboxesPerGPU represents the maximum of mandelboxes we can assign to
// each GPU and still maintain acceptable performance. Note that we are
// assuming that all GPUs on a mandelbox are uniform (and that we are using
// only one instance type).
const MaxMandelboxesPerGPU = 1

// usagePerGPU contains the number of mandelboxes currently assigned to a
// each GPU (indexed by the GPU's Index).
var usagePerGPU []uint8

// Lock to protect `usagePerGPU`.
var usageLock = new(sync.Mutex)

// Allocate allocates a GPU with room for a mandelbox and returns its index.
func Allocate() (Index, error) {
	usageLock.Lock()
	defer usageLock.Unlock()

	// We try to allocate as evenly as possible.
	var minIndex = Index(-1)
	var minVal uint8 = math.MaxUint8
	for i, v := range usagePerGPU {
		// Just skip full GPUs.
		if v >= MaxMandelboxesPerGPU {
			continue
		}
		if v < minVal {
			minVal = v
			minIndex = Index(i)
		}
	}

	if minIndex == Index(-1) || minVal == math.MaxUint8 {
		return -1, utils.MakeError("No free GPUs!")
	}

	// Allocate slot on GPU and return Index
	usagePerGPU[minIndex]++
	return minIndex, nil
}

// Free decrements the number of mandelboxes we consider assigned to a given
// GPU indexed by `index`.
func Free(index Index) error {
	usageLock.Lock()
	defer usageLock.Unlock()

	if usagePerGPU[index] <= 0 {
		return utils.MakeError("Free called on a GPU Index that has no mandelboxes allocated!")
	}

	usagePerGPU[index]--
	return nil
}
