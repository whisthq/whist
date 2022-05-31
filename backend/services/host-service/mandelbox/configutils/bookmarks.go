package configutils


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
