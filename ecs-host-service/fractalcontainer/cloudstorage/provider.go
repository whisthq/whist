package cloudstorage

import (
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

type Provider string

const (
	GoogleDrive Provider = "google_drive"
	Dropbox     Provider = "dropbox"
)

func (p Provider) PrettyName() (string, error) {
	switch p {
	case GoogleDrive:
		return "Google Drive", nil
	case Dropbox:
		return "Dropbox", nil
	default:
		return "", logger.MakeError("PrettyName: bad provider: %s", p)
	}
}

func (p Provider) RcloneInternalName() (string, error) {
	switch p {
	case GoogleDrive:
		return "drive", nil
	case Dropbox:
		return "dropbox", nil
	default:
		return "", logger.MakeError("RcloneInternalName: bad provider: %s", p)
	}
}

func GetProvider(name string) (Provider, error) {
	switch name {
	case string(GoogleDrive):
		return GoogleDrive, nil
	case string(Dropbox):
		return Dropbox, nil
	default:
		return "", logger.MakeError("cloudstorage.GetProvider: bad provider name: %s", name)
	}
}
