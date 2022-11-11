package config

import (
	"flag"
	"fmt"
	"os"
	"strings"
	"time"

	"github.com/knadh/koanf"
	"github.com/knadh/koanf/parsers/toml"
	"github.com/knadh/koanf/providers/basicflag"
	"github.com/knadh/koanf/providers/env"
	"github.com/knadh/koanf/providers/file"
)

func getConfigFromFile(k *koanf.Koanf) error {
	// Load TOML config
	err := k.Load(file.Provider("config.toml"), toml.Parser())
	if err != nil {
		return fmt.Errorf("error loading config: %s", err)
	}

	config.enabledRegions = k.MapKeys("regions")
	config.mandelboxLimitPerUser = k.Int("mandelboxes.limit")

	// Convert to uint32 since target mandelboxes can't be negative
	config.targetFreeMandelboxes = make(map[string]uint32)
	for k, v := range k.IntMap("regions") {
		config.targetFreeMandelboxes[k] = uint32(v)
	}

	return nil
}

func getConfigFromFlags(k *koanf.Koanf) error {
	// Load command line config
	f, err := setFlags()
	if err != nil {
		return fmt.Errorf("error parsing flags: %s", err)
	}

	err = k.Load(basicflag.Provider(f, "."), nil)
	if err != nil {
		return fmt.Errorf("error loading flags to config: %s", err)
	}

	config.scalingEnabled = k.Bool("scaling")
	config.scalingInterval = k.Duration("scalingtime")
	config.commitHashOverride = k.Bool("commitoverride")
	config.useProdLogging = k.Bool("useProdLogging")

	return nil
}

func getConfigFromEnv(k *koanf.Koanf) error {
	err := k.Load(env.Provider("DATABASE_URL", ".", func(s string) string {
		return strings.Replace(strings.ToLower(s), "_", ".", -1)
	}), nil)
	if err != nil {
		return fmt.Errorf("error loading flags to config: %s", err)
	}

	config.databaseConnectionString = k.String("database.url")
	return nil
}

func setFlags() (*flag.FlagSet, error) {
	f := flag.NewFlagSet("config", flag.ContinueOnError)
	f.Duration("scalingtime", time.Duration(time.Minute*10), "Interval between when each scaling service runs.")
	f.Duration("cleantime", time.Duration(time.Minute), "Interval between when each cleaner service runs.")

	f.Bool("scaling", false, "If the application starts the scaling service or not.")
	f.Bool("cleanup", false, "If the application starts the cleaner service or not.")
	f.Bool("prodlogging", false, `If the application sends logs to Logz.io and reports errors to Sentry. If this option is passed 
the SENTRY_DSN and LOGZIO_SHIPPING_TOKEN env vars must be defined.`)
	f.Bool("commitoverride", true, "Setting this flag makes the assign action use the latest commit hash from the database if the override value is sent in the request.")

	err := f.Parse(os.Args[1:])
	if err != nil {
		return nil, err
	}

	return f, nil
}
