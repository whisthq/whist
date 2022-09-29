package scaling_algorithms

import (
	"reflect"
	"testing"

	"github.com/whisthq/whist/backend/services/constants"
)

// TestGenerateInstanceCapacityMap tests that the generateInstanceCapacityMap
// function returns the correct map of instance capacities.
func TestGenerateInstanceCapacityMap(t *testing.T) {
	cases := []struct {
		name     string
		gpuMap   map[string]int
		vcpuMap  map[string]int
		expected map[string]int
	}{
		{
			name: "gpu limited",
			gpuMap: map[string]int{
				"a": 1,
				"b": 2,
			},
			vcpuMap: map[string]int{
				"a": 1000,
				"b": 2000,
			},
			expected: map[string]int{
				"a": constants.MaxMandelboxesPerGPU,
				"b": 2 * constants.MaxMandelboxesPerGPU,
			},
		},
		{
			name: "vcpu limited",
			gpuMap: map[string]int{
				"a": 1000,
				"b": 2000,
			},
			vcpuMap: map[string]int{
				"a": 4,
				"b": 8,
			},
			expected: map[string]int{
				"a": 4 / VCPUsPerMandelbox,
				"b": 8 / VCPUsPerMandelbox,
			},
		},
		{
			name: "no overlap",
			gpuMap: map[string]int{
				"a": 1,
				"b": 2,
			},
			vcpuMap: map[string]int{
				"c": 4,
				"d": 8,
			},
			expected: map[string]int{},
		},
	}

	for _, testCase := range cases {
		t.Run(testCase.name, func(t *testing.T) {
			actual := generateInstanceCapacityMap(testCase.gpuMap, testCase.vcpuMap)
			if !reflect.DeepEqual(actual, testCase.expected) {
				t.Errorf("Expected %v, got %v", testCase.expected, actual)
			}
		})
	}
}
