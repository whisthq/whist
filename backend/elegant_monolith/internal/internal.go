package internal

import "time"

type Service interface {
	Start()
	Stop()
}

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

func (Instance) TableName() string {
	return "whist.instances"
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

func (Mandelbox) TableName() string {
	return "whist.mandelboxes"
}

type Image struct {
	ID        string
	Provider  string
	Region    string
	ClientSHA string
	UpdatedAt time.Time
}

func (Image) TableName() string {
	return "whist.images"
}

const (
	MandelboxStatusWaiting    string = "WAITING"
	MandelboxStatusAllocated  string = "ALLOCATED"
	MandelboxStatusConnecting string = "CONNECTING"
	MandelboxStatusDying      string = "DYING"
)
