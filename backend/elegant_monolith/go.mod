module github.com/whisthq/whist/backend/elegant_monolith

go 1.19

replace (
	github.com/whisthq/whist/backend/elegant_monolith => ./
	github.com/whisthq/whist/backend/elegant_monolith/assign => ./assign
	github.com/whisthq/whist/backend/elegant_monolith/deploy => ./deploy
	github.com/whisthq/whist/backend/elegant_monolith/internal => ./internal
	github.com/whisthq/whist/backend/elegant_monolith/pkg => ./pkg
	github.com/whisthq/whist/backend/elegant_monolith/scaling => ./scaling
	github.com/whisthq/whist/backend/elegant_monolith/verify => ./verify
)

require (
	github.com/google/uuid v1.1.2
	github.com/knadh/koanf v1.4.4
	google.golang.org/grpc v1.50.1
	google.golang.org/protobuf v1.28.1
)

require (
	github.com/fsnotify/fsnotify v1.4.9 // indirect
	github.com/golang/protobuf v1.5.2 // indirect
	github.com/mitchellh/copystructure v1.2.0 // indirect
	github.com/mitchellh/mapstructure v1.5.0 // indirect
	github.com/mitchellh/reflectwalk v1.0.2 // indirect
	github.com/pelletier/go-toml v1.7.0 // indirect
	golang.org/x/net v0.0.0-20210410081132-afb366fc7cd1 // indirect
	golang.org/x/sys v0.0.0-20210603081109-ebe580a85c40 // indirect
	golang.org/x/text v0.3.6 // indirect
	google.golang.org/genproto v0.0.0-20210602131652-f16073e35f0c // indirect
)
