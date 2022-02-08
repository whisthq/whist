package configutils

type Bookmark struct {
	DateAdded string `json:"date_added,omitempty"`
	Guid      string `json:"guid,omitempty"`
	Id        string `json:"id,omitempty"`
	Name      string `json:"name,omitempty"`
	Type      string `json:"type,omitempty"`
	Url       string `json:"url,omitempty"`
}

type BookmarkFolder struct {
	Children     []Bookmark `json:"children,omitempty"`
	DateAdded    string     `json:"date_added,omitempty"`
	DateModified string     `json:"date_modified,omitempty"`
	Guid         string     `json:"guid,omitempty"`
	Id           string     `json:"id,omitempty"`
	Name         string     `json:"name,omitempty"`
	Type         string     `json:"type,omitempty"`
}

type Bookmarks struct {
	Checksum     string           `json:"checksum,omitempty"`
	Roots        []BookmarkFolder `json:"roots,omitempty"`
	SyncMetadata string           `json:"sync_metadata,omitempty"`
	Version      int              `json:"version,omitempty"`
}
