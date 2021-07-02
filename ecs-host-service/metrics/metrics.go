// Package metrics is responsible for computing metrics about the instance during
// host service runtime. This includes metrics about total load, CPU usage,
// etc.
package metrics // import "github.com/fractal/fractal/ecs-host-service/metrics"

import (
	"math/rand"
	"sync"
	"time"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/utils"

	"github.com/NVIDIA/go-nvml/pkg/nvml"

	"github.com/shirou/gopsutil/cpu"
	"github.com/shirou/gopsutil/disk"
	"github.com/shirou/gopsutil/host"
	"github.com/shirou/gopsutil/load"
	"github.com/shirou/gopsutil/mem"
)

// A RuntimeMetrics groups together several pieces of useful information about
// the underlying host.
type RuntimeMetrics struct {
	// Memory

	// TotalMemoryKB represents the total amount of memory on the host, in KB.
	TotalMemoryKB uint64

	// FreeMemoryKB represents the amount of currently free memory on the host,
	// in KB. This is a lower bound for how much more memory we can theoretically
	// use, since some cached files can be evicted to free up some more memory.
	FreeMemoryKB uint64

	// AvailableMemoryKB represents the amount of currently available memory in
	// the host in KB. This is a hard upper bound for how much more memory we can
	// use before we start swapping or crash. We should allocate well below this
	// number because performance will suffer long before we get here.
	AvailableMemoryKB uint64

	// CPU

	// LogicalCPUs represents the number of logical CPUs on the host when the
	// host service was started.
	LogicalCPUs int

	// CPUUtilizationPercent represents the percentage of total CPU utilization
	// across the host system.
	CPUUtilizationPercent float64

	// NanoCPUsRemaining represents the amount of CPU remaining in units of
	// (10^-9 CPU). In other words, if two cores are fully idle and all others
	// are at maximum usage, there's 2*10^9 NanoCPUsRemaining on that instance.
	NanoCPUsRemaining uint64

	// GPU

	// NumberOfGPUs represents the number of NVIDIA GPUs on the host system. Note
	// that for now we assume that all GPUs on an instance are homogeneous.
	NumberOfGPUs int

	// TotalVideoMemoryKB represents the amount of video memory in total across
	// all GPUs on the instance.
	TotalVideoMemoryKB uint64

	// UsedVideoMemoryKB represents the amount of in-use video memory totaled
	// across all GPUs on the instance.
	UsedVideoMemoryKB uint64

	// FreeVideoMemoryKB represents the amount of video memory remaining in
	// total across all GPUs on the instance.
	FreeVideoMemoryKB uint64

	// GPUUtilizationPercent represents (an imprecise estimate of) the percentage
	// of time the GPUs recently spent on GPU kernel computations (this does not
	// refer to the Linux kernel, but rather kernels that can run on a GPU).
	GPUKernelUtilizationPercent float64

	// GPUMemoryBandwidthUtilizationPercent represents (an imprecise estimate of)
	// the percentage of time recently when the GPU's memory was being written
	// to.
	GPUMemoryBandwidthUtilizationPercent float64

	// Load Averages

	// For an explanation of what these mean, see
	// https://www.brendangregg.com/blog/2017-08-08/linux-load-averages.html
	LoadAverage1Min  float64
	LoadAverage5Min  float64
	LoadAverage15Min float64

	// Disk Usage

	// DiskTotalKB represents the total amount of SSD space available to the
	// host.
	DiskTotalKB uint64

	// DiskUsedKB represents the total amount of SSD space in use on the host.
	DiskUsedKB uint64

	// DiskFreeKB represents the total amount of SSD space free on the host.
	DiskFreeKB uint64

	// DiskUsedPercent represents the percentage of disk space used on the host.
	DiskUsedPercent float64

	// Time

	// Uptime represents how long the current instance has been up, at a
	// granularity of one second.
	Uptime time.Duration

	// TimeStamp denotes the timestamp when the values in this struct were
	// collected. This can be used by callers of `GetLatest` to determine how
	// out-of-date the stats are.
	TimeStamp time.Time
}

func init() {
	err := startCollectionGoroutine(30 * time.Second)
	if err != nil {
		// We can safely do a "real" panic in an init function.
		logger.Panicf(nil, "Error starting metrics collection goroutine: %s", err)
	}
}

// As long as this channel is blocking, we should keep collecting metrics. As
// soon as the channel is closed, we stop collecting metrics.
var collectionKeepAlive = make(chan interface{}, 1)

// startCollectionGoroutine starts a goroutine that regularly computes metrics about the
// host and caches the most recent result. This prevents requests for metrics
// (like average CPU usage) from blocking, since this goroutine can do the
// blocking instead. The frequency passed must be at least 1.5 seconds to avoid
// performance issues, and to allow for adding a random jitter to the
// frequency. Note that you must wait for StartCollection to return
// successfully before calling any metrics-retrieving functions.
func startCollectionGoroutine(frequency time.Duration) error {
	if frequency < 1500*time.Millisecond {
		return utils.MakeError("The frequency passed to StartMetricsCollection must be at least one 1.5 seconds to avoid performance issues.")
	}

	// Initialize necessary libraries (i.e. NVML). Don't forget to uninitialize
	// them below (we can't use defer here since we're returning from this
	// function).
	if nvmlRet := nvml.Init(); nvmlRet != nvml.SUCCESS {
		return utils.MakeError("Unable to initialize NVML for metrics collection.")
	}

	// We do the first batch of metrics collection before returning, so that any
	// calls to metrics-retrieving functions after StartMetricsCollection returns
	// are guaranteed to succeed. Note that the very first batch of CPU data is
	// not likely to be accurate, since we're measuring over a zero-length
	// interval, but this is not a big deal. We also don't have to worry about
	// locking here, since clients of this library are required to call
	// StartCollection before anything else.
	latestMetrics, latestErrors = collectOnce()
	if len(latestErrors) != 0 {
		return utils.MakeError("Errors collecting the first batch of metrics: %v", latestErrors)
	}

	// Start the metrics collection goroutine
	go func() {
		logger.Infof("Starting metrics collection goroutine.")
		timerChan := make(chan interface{})

		for {
			// We add some jitter to this sleeptime to avoid unintended effects or
			// patterns from recording data at a constant frequency.
			sleepTime := frequency + time.Second - time.Duration(rand.Intn(2000))*time.Millisecond
			timer := time.AfterFunc(sleepTime, func() { timerChan <- struct{}{} })

			select {
			case _, _ = <-collectionKeepAlive:
				// If we hit this case, that means that `collectionKeepAlive` was either
				// closed or written to (it should not be written to), but either way,
				// it's time to die.

				logger.Infof("Shutting down metrics collection goroutine.")

				// Uninitialize libraries (i.e. NVML).
				if nvmlRet := nvml.Shutdown(); nvmlRet != nvml.SUCCESS {
					logger.Errorf("Error shutting down NVML library.")
				}

				// Stop timer to avoid leaking a goroutine (not that it matters if we're
				// shutting down, but still).
				// (https://golang.org/pkg/time/#Timer.Stop)
				if !timer.Stop() {
					<-timer.C
				}
				return

			case _ = <-timerChan:
				// Time to collect some metrics
				newMetrics, errs := collectOnce()

				latestLock.Lock()
				latestMetrics, latestErrors = newMetrics, errs
				latestLock.Unlock()

				logger.Infof("Collected latest metrics: %+v", newMetrics)
				if len(latestErrors) != 0 {
					logger.Errorf("Errors collecting latest metrics: %v", latestErrors)
				}
			}
		}
	}()

	return nil
}

// latestMetrics is the cache of the most recently measured metrics.
var latestMetrics RuntimeMetrics

// latestErrors is the cache of errors encountered while collecting the most
// recently measured metrics.
var latestErrors []error

// latestLock protects latestMetrics and latestErrors so that they can be updated and read concurrently.
var latestLock sync.Mutex

// collectOnce attempts to collect all the metrics it can, and caches
// the results in `latestMetrics`. It also returns a slice of all the errors
// that it encountered. This is by design, since we don't want a single error
// to prevent the collection of all metrics. In other words, this function
// tries to recover and do as much as it can, even in the face of errors.
func collectOnce() (RuntimeMetrics, []error) {
	newMetrics := RuntimeMetrics{}
	errs := make([]error, 0)

	// We organize the code to collect metrics in the same order that they occur
	// in the definition of RuntimeMetrics for code organization purposes. Note
	// that wherever possible, we also populate the returned RuntimeMetrics
	// struct with invalid data whenever there's an error collecting a particular
	// piece of data (e.g. -1 for the recent load). This should make it clear on
	// a graph, for instance, that our data collection process has run into
	// errors, and at least prevent us from reaching "sensible" conclusions from
	// bogus data.

	// Memory stats
	if memStats, err := mem.VirtualMemory(); err != nil {
		errs = append(errs, utils.MakeError("Error getting memory stats: %s", err))
	} else {
		newMetrics.TotalMemoryKB = memStats.Total / 1024
		newMetrics.FreeMemoryKB = memStats.Free / 1024
		newMetrics.AvailableMemoryKB = memStats.Available / 1024
	}

	// CPU stats
	if cpuCount, err := cpu.Counts(true); err != nil {
		errs = append(errs, utils.MakeError("Error getting Logical CPU count: %s", err))
	} else {
		newMetrics.LogicalCPUs = cpuCount

		cpuPercent, err := cpu.Percent(0, false)
		if err != nil {
			errs = append(errs, utils.MakeError("Error getting CPU utilization: %s", err))
			newMetrics.CPUUtilizationPercent = -1
		} else {
			newMetrics.CPUUtilizationPercent = cpuPercent[0]
			newMetrics.NanoCPUsRemaining = uint64(cpuCount) * uint64(1e9-1e7*cpuPercent[0])
		}
	}

	// GPU stats
	if numGPUs, nvmlRet := nvml.DeviceGetCount(); nvmlRet != nvml.SUCCESS {
		errs = append(errs, utils.MakeError("Error getting GPU count"))
		newMetrics.NumberOfGPUs = -1
	} else {
		newMetrics.NumberOfGPUs = numGPUs

		var totalVramKB uint64 = 0
		var usedVramKB uint64 = 0
		var totalKernelUtilPercent uint32 = 0
		var totalMemBandwidthUtilPercent uint32 = 0
		for i := 0; i < numGPUs; i++ {
			if device, nvmlRet := nvml.DeviceGetHandleByIndex(i); nvmlRet != nvml.SUCCESS {
				errs = append(errs, utils.MakeError("Couldn't get NVIDIA device at index %v", i))
				break
			} else {
				if memInfo, nvmlRet := nvml.DeviceGetMemoryInfo(device); nvmlRet != nvml.SUCCESS {
					errs = append(errs, utils.MakeError("Couldn't get video memory info for device at NVML index %v", i))
					break
				} else {
					totalVramKB += memInfo.Total / 1024
					usedVramKB += memInfo.Used / 1024
					// For some reason, NVML always returns 0 for memInfo.Free, so we do
					// the free calculation ourselves below.
				}

				if util, nvmlRet := nvml.DeviceGetUtilizationRates(device); nvmlRet != nvml.SUCCESS {
					errs = append(errs, utils.MakeError("Couldn't get utilization info for device at NVML index %v", i))
					break
				} else {
					totalKernelUtilPercent += util.Gpu
					totalMemBandwidthUtilPercent += util.Memory
				}
			}
		}

		newMetrics.TotalVideoMemoryKB = totalVramKB
		newMetrics.UsedVideoMemoryKB = usedVramKB
		newMetrics.FreeVideoMemoryKB = totalVramKB - usedVramKB

		newMetrics.GPUKernelUtilizationPercent = float64(totalKernelUtilPercent) / float64(numGPUs)
		newMetrics.GPUMemoryBandwidthUtilizationPercent = float64(totalMemBandwidthUtilPercent) / float64(numGPUs)
	}

	// Load stats
	if avgStat, err := load.Avg(); err != nil {
		errs = append(errs, utils.MakeError("Couldn't get average load statistics: %s", err))
		newMetrics.LoadAverage1Min = -1
		newMetrics.LoadAverage5Min = -1
		newMetrics.LoadAverage15Min = -1
	} else {
		newMetrics.LoadAverage1Min = avgStat.Load1
		newMetrics.LoadAverage5Min = avgStat.Load5
		newMetrics.LoadAverage15Min = avgStat.Load15
	}

	// Disk stats
	if diskStat, err := disk.Usage("/"); err != nil {
		errs = append(errs, utils.MakeError("Couldn't get disk usage stats: %s", err))
		newMetrics.DiskUsedPercent = -1
	} else {
		newMetrics.DiskTotalKB = diskStat.Total / 1024
		newMetrics.DiskUsedKB = diskStat.Used / 1024
		newMetrics.DiskFreeKB = diskStat.Free / 1024
		newMetrics.DiskUsedPercent = diskStat.UsedPercent
	}

	// Time stats
	if uptimeSecs, err := host.Uptime(); err != nil {
		errs = append(errs, utils.MakeError("Couldn't get uptime for host: %s", err))
	} else {
		newMetrics.Uptime = time.Second * time.Duration(uptimeSecs)
	}
	newMetrics.TimeStamp = time.Now().UTC()

	return newMetrics, errs
}

// GetLatest returns the most recently collected metrics for the host instance,
// as well as any errors encountered while collecting those metrics. All
// metrics are refreshed at approximately the frequency passed into
// StartCollection.
func GetLatest() (RuntimeMetrics, []error) {
	latestLock.Lock()
	defer latestLock.Unlock()
	return latestMetrics, latestErrors
}

// Close stops the metrics collection goroutine and leaves behind an error for
// anyone who attempts to access the metrics after it is called.
func Close() {
	close(collectionKeepAlive)

	latestLock.Lock()
	defer latestLock.Unlock()
	latestErrors = []error{
		utils.MakeError("Metrics-collection goroutine has been stopped."),
	}
}
