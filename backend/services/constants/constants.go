// Package constants constains shared constants between the various
// backend services.
package constants

// MaxMandelboxesPerGPU represents the maximum of mandelboxes we can assign to
// each GPU and still maintain acceptable performance. Note that we are
// assuming that all GPUs on a mandelbox are uniform (and that we are using
// only one instance type).
const MaxMandelboxesPerGPU = 3
