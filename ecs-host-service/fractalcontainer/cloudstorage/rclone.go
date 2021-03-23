package cloudstorage // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"

import (
	"encoding/json"
	"os"
	"os/exec"
	"strings"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// GenerateRcloneToken creates the configuration that is passed to `rclone
// config create`.
func GenerateRcloneToken(AccessToken string, RefreshToken string, Expiry string, TokenType string) ([]byte, error) {
	buf, err := json.Marshal(
		struct {
			AccessToken  string `json:"access_token"`
			TokenType    string `json:"token_type"`
			RefreshToken string `json:"refresh_token"`
			Expiry       string `json:"expiry"`
		}{
			AccessToken,
			TokenType,
			RefreshToken,
			Expiry,
		},
	)
	if err != nil {
		return nil, logger.MakeError("Error creating token for rclone: %s", err)
	}
	return buf, nil
}

// CreateRcloneConfig runs `rclone config create` to later be able to mount the
// given cloud storage account.  NOTE: CreateRcloneConfig does not sanitize
// configName or rcloneToken!
func CreateRcloneConfig(configName string, provider Provider, rcloneToken []byte, clientID string, clientSecret string) error {
	providerType, err := provider.RcloneInternalName()
	if err != nil {
		return logger.MakeError("Couldn't create rclone config. Error: %s", err)
	}

	strcmd := strings.Join(
		[]string{
			"#!/bin/sh\n\n" +
				"/usr/bin/rclone", "config", "create", configName, providerType,
			"config_is_local", "false",
			"config_refresh_token", "false",
			"token", logger.Sprintf("'%s'", rcloneToken),
			"client_id", clientID,
			"client_secret", clientSecret,
		}, " ")

	scriptpath := logger.Sprintf("/fractal/temp/config-create-%s.sh", configName)

	f, err := os.Create(scriptpath)
	if err != nil {
		return logger.MakeError("Couldn't create rclone config script at %s. Error: %s", scriptpath, err)
	}

	n, err := f.WriteString(strcmd)
	if err != nil {
		return logger.MakeError("Couldn't write to rclone config script at %s. Error: %s", scriptpath, err)
	} else if n < len(strcmd) {
		return logger.MakeError("Didn't write complete rclone config script at %s. Only wrote %d of %d characters.", scriptpath, n, len(strcmd))
	}

	os.Chmod(scriptpath, 0700)
	f.Close()
	defer os.RemoveAll(scriptpath)
	cmd := exec.Command(scriptpath)

	output, err := cmd.CombinedOutput()
	if err != nil {
		return logger.MakeError("Could not run \"rclone config create\" command: %s. Output: %s", err, output)
	}

	logger.Info("Ran \"rclone config create\" command with output: %s", output)
	return nil
}
