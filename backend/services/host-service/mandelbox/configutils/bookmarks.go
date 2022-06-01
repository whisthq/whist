package configutils

import (
	"encoding/json"

	"github.com/whisthq/whist/backend/services/types"
)

// This type defines a standard Chromium Bookmark object
type Bookmark struct {
	Children     []Bookmark `json:"children,omitempty"`
	DateAdded    string     `json:"date_added,omitempty"`
	DateModified string     `json:"date_modified,omitempty"`
	Guid         string     `json:"guid,omitempty"`
	Id           string     `json:"id,omitempty"`
	Name         string     `json:"name,omitempty"`
	Type         string     `json:"type,omitempty"`
	Url          string     `json:"url,omitempty"`
}

// This type defines a collection of many Bookmark objects
type Bookmarks struct {
	Checksum     string              `json:"checksum,omitempty"`
	Roots        map[string]Bookmark `json:"roots,omitempty"`
	SyncMetadata string              `json:"sync_metadata,omitempty"`
	Version      int                 `json:"version,omitempty"`
}

// UnmarshalBookmarks takes a JSON string containing bookmark data
// and unmarshals it into a Bookmarks struct, returning the struct
// and any errors encountered.
func UnmarshalBookmarks(bookmarks types.Bookmarks) (Bookmarks, error) {
	var bookmarksObj Bookmarks
	err := json.Unmarshal([]byte(bookmarks), &bookmarksObj)
	return bookmarksObj, err
}
