package dbclient

import "time"

type Instance struct {
	ID                string
	Provider          string
	Region            string
	ImageID           string
	ClientSHA         string
	IPAddress         string
	Type              string
	RemainingCapacity string
	Status            string
	CreatedAt         time.Time
	UpdatedAt         time.Time
}

type Mandelbox struct {
	ID         string
	App        string
	InstanceID string
	UserID     string
	SessionID  string
	Status     string
	CreatedAt  time.Time
	UpdatedAt  time.Time
}

type Image struct {
	ID        string
	Provider  string
	Region    string
	ClientSHA string
	UpdatedAt time.Time
}
