package scaling_algorithms

import (
	"reflect"
	"testing"

	"github.com/whisthq/whist/backend/services/constants"
	"github.com/whisthq/whist/backend/services/metadata"
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

// TestEnabledRegionsMap tests the GetEnabledRegions function that
// returns a list of regions according to the environment.
func TestEnabledRegionsMap(t *testing.T) {
	var tests = []struct {
		env  string
		want []string
	}{
		{"dev", []string{"us-east-1"}},
		{"staging", []string{"us-east-1"}},
		{"prod", []string{
			"us-east-1",
			"us-east-2",
			"us-west-1",
			"us-west-2",
			"ca-central-1",
			"eu-west-2",
			"eu-central-1",
			"ap-south-1",
		}},
		{"localdev", []string{"us-east-1"}},
		{"unknown", []string{"us-east-1"}},
	}

	for _, tt := range tests {
		t.Run(tt.env, func(t *testing.T) {

			// Mock the GetAppEnvironment function to return the tested environment
			metadata.GetAppEnvironment = func() metadata.AppEnvironment {
				return metadata.AppEnvironment(tt.env)
			}

			got := GetEnabledRegions()
			ok := reflect.DeepEqual(got, tt.want)
			if !ok {
				t.Errorf("Enabled regions map is incorrect, got %v, expected %v", got, tt.want)
			}
		})
	}

}
