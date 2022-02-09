package configutils

import (
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/whisthq/whist/backend/services/types"
)

// TestUnmarshalValidBookmarks tests that a valid bookmark
// JSON string (with special characters) is properly unmarshalled
// into a Bookmarks struct.
func TestUnmarshalValidBookmarks(t *testing.T) {
	// Valid bookmark JSON
	validBookmarks := `{
		"checksum": "",
		"roots": {
			"bookmark_bar": {
				"children": [
					{
						"date_added": "1527180903",
						"date_modified": "1527180903",
						"guid": "2",
						"id": "2",
						"name": "😂",
						"type": "url",
						"url": "https://www.example.com/"
					},
					{
						"children": [
							{
								"date_added": "1527180903",
								"date_modified": "1527180903",
								"guid": "4",
								"id": "4",
								"name": "网站",
								"type": "url",
								"url": "https://www.example.com/"
							}
						],
						"date_added": "1527180903",
						"date_modified": "1527180903",
						"guid": "3",
						"id": "3",
						"name": "Nested",
						"type": "folder"
					}
				],
				"date_added": "1527180903",
				"date_modified": "1527180903",
				"guid": "1",
				"id": "1",
				"name": "Bookmark Bar",
				"type": "folder"
			},
			"other": {
				"children": [ ],
				"date_added": "12345",
				"name": "Other Bookmarks",
				"type": "folder"
			}
		},
		"sync_metadata": "adfsd",
		"version": 1
	}`

	expectedBookmarks := Bookmarks{
		Roots: map[string]Bookmark{
			"bookmark_bar": {
				Children: []Bookmark{
					{
						DateAdded:    "1527180903",
						DateModified: "1527180903",
						Guid:         "2",
						Id:           "2",
						Name:         "😂",
						Type:         "url",
						Url:          "https://www.example.com/",
					},
					{
						Children: []Bookmark{
							{
								DateAdded:    "1527180903",
								DateModified: "1527180903",
								Guid:         "4",
								Id:           "4",
								Name:         "网站",
								Type:         "url",
								Url:          "https://www.example.com/",
							},
						},
						DateAdded:    "1527180903",
						DateModified: "1527180903",
						Guid:         "3",
						Id:           "3",
						Name:         "Nested",
						Type:         "folder",
					},
				},
				DateAdded:    "1527180903",
				DateModified: "1527180903",
				Guid:         "1",
				Id:           "1",
				Name:         "Bookmark Bar",
				Type:         "folder",
			},
			"other": {
				Children:  []Bookmark{},
				DateAdded: "12345",
				Name:      "Other Bookmarks",
				Type:      "folder",
			},
		},
		SyncMetadata: "adfsd",
		Version:      1,
	}

	bookmarks, err := UnmarshalBookmarks(types.Bookmarks(validBookmarks))
	if err != nil {
		t.Fatalf("UnmarshalBookmarks returned an unexpected error: %v", err)
	}

	if !cmp.Equal(bookmarks, expectedBookmarks) {
		t.Errorf("UnmarshalBookmarks returned %v, expected %v", bookmarks, expectedBookmarks)
	}
}

// TestUnmarshalInvalidBookmarks tests that an invalid bookmark
// JSON string returns an error as expected.
func TestUnmarshalInvalidBookmarks(t *testing.T) {
	invalidBookmarks := "invalid"
	bookmarks, err := UnmarshalBookmarks(types.Bookmarks(invalidBookmarks))

	// Check that all fields are empty
	if len(bookmarks.Roots) > 0 {
		t.Errorf("UnmarshalBookmarks returned roots %v, expected %v", bookmarks.Roots, nil)
	}

	if bookmarks.SyncMetadata != "" {
		t.Errorf("UnmarshalBookmarks returned sync_metadata %v, expected %v", bookmarks.SyncMetadata, "")
	}

	if bookmarks.Version != 0 {
		t.Errorf("UnmarshalBookmarks returned version %v, expected %v", bookmarks.Version, 0)
	}

	if bookmarks.Checksum != "" {
		t.Errorf("UnmarshalBookmarks returned checksum %v, expected %v", bookmarks.Checksum, "")
	}

	if err == nil {
		t.Errorf("UnmarshalBookmarks returned no error, expected an error")
	}
}
