//go:build !linux

package metrics

// This file defines the functions necessary for GPU metrics-dependent code to
// compile on non-Linux platforms. All functions in this file are no-ops.
// Because the Go code using NVML doesn't compile on an M1 MacBook, we need
// platform-specific implementations of the GPU files so that we can compile
// the host service on MacBooks (to avoid spinning up an expensive g4dn instance
// just to make a quick change).

func initializeGPUMetricsCollector() error {
	return nil
}

func shutdownGPUMetricsCollector() {}

func (newMetrics *RuntimeMetrics) collectGPUMetrics() []error {
	return []error{}
}
