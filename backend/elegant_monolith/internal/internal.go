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
	IPAddress         string `gorm:"column:ip_addr"`
	Type              string `gorm:"column:instance_type"`
	RemainingCapacity int
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
	ID        string `gorm:"column:image_id"`
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
