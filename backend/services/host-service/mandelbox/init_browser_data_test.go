package mandelbox

import (
	"bytes"
	"os"
	"path"
	"testing"

	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	"github.com/whisthq/whist/backend/services/types"
)

func TestTestUserInitialBrowserWrite(t *testing.T) {
	validJSON := `{
		"cookies": [
			{"creation_utc": 13280861983875934, "host_key": "test_host_key_1.com"}
			{"creation_utc": 4228086198342934, "host_key": "test_host_key_2.com"}
		],
		"extensions": "not_real_extension_id,not_real_second_extension_id"
	 }`
	var tests = []struct {
		name, browserData string
		err               bool
		expected          string
	}{
		{"Existing Browser Data", validJSON, false, validJSON},
		{"Empty Browser Data", "", true, ""},
		{"Malformed Browser Data", `{
			cookies: [
				{creation_utc: 13280861983875934, host_key: test_host_key_1.com}
				{creation_utc: 4228086198342934, 'host_key': test_host_key_2.com}
			],
			extensions: not_real_extension_id,not_real_second_extension_id
		 }`, true, ""},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testMbox, _, _ := createTestMandelboxData()

			// Reset filesystem now, and at the end of this test
			os.RemoveAll(testMbox.GetUserConfigDir())
			defer os.RemoveAll(testMbox.GetUserConfigDir())

			err := testMbox.WriteUserInitialBrowserData(types.BrowserData(tt.browserData))
			if err != nil && !tt.err {
				t.Fatalf("did not expect error, got: %s", err)
			}

			deflatedBrowserData, err := configutils.GzipDeflateString(tt.browserData)
			if err != nil {
				t.Fatalf("could not deflate browser data: %v", err)
			}
			if err := testMbox.WriteUserInitialBrowserData(types.BrowserData(deflatedBrowserData)); err != nil {
				t.Fatalf("error writing user initial browser data: %v", err)
			}

			// Get browser data file path
			browserDataFile := path.Join(testMbox.GetUserConfigDir(), UnpackedConfigsDirectoryName, UserInitialBrowserFile)

			matchingFile, err := os.Open(browserDataFile)
			if err != nil {
				t.Fatalf("error opening matching file %s: %v", browserDataFile, err)
			}

			var matchingFileBuf bytes.Buffer
			_, err = matchingFileBuf.ReadFrom(matchingFile)
			if err != nil {
				t.Fatalf("error reading matching file %s: %v", browserDataFile, err)
			}

			// Check contents match
			if string(tt.expected) != matchingFileBuf.String() {
				t.Fatalf("file contents don't match for file %s: got: %s, expected: %s", browserDataFile, matchingFileBuf.Bytes(), tt.expected)
			}
		})
	}
}
