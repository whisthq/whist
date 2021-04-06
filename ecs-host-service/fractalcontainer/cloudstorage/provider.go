package cloudstorage // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"

import (
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// Provider represents a cloud storage provider that can be used with rlcone.
// Note that Providers have several names --- a "canonical" name which is
// stored in the underlying string, a "pretty" one which faces users, and a
// "RcloneInternal" name, which is how `rclone` refers to that provider.
type Provider string

// This block contains all the cloud storage providers that Fractal currently
// supports.
const (
	GoogleDrive Provider = "google_drive"
	Dropbox     Provider = "dropbox"
)

// PrettyName returns the "pretty" name of a cloud storage provider.
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

// RcloneInternalName returns the "RcloneInternal" name of a cloud storage provider.
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

// GetProvider returns a Provider given its "canonical" name.
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
