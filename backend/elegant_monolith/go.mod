module github.com/whisthq/whist/backend/elegant_monolith

go 1.19

replace (
	github.com/whisthq/whist/backend/elegant_monolith => ./
	github.com/whisthq/whist/backend/elegant_monolith/assign => ./assign
	github.com/whisthq/whist/backend/elegant_monolith/deploy => ./deploy
	github.com/whisthq/whist/backend/elegant_monolith/internal => ./internal
	github.com/whisthq/whist/backend/elegant_monolith/pkg => ./pkg
	github.com/whisthq/whist/backend/elegant_monolith/pkg/dbclient => ./pkg/dbclient
	github.com/whisthq/whist/backend/elegant_monolith/scaling => ./scaling
	github.com/whisthq/whist/backend/elegant_monolith/verify => ./verify
)

require (
	github.com/getsentry/sentry-go v0.14.0
	github.com/knadh/koanf v1.4.4
	github.com/logzio/logzio-go v1.0.6
	go.uber.org/zap v1.17.0
	golang.org/x/exp v0.0.0-20221108223516-5d533826c662
	google.golang.org/grpc v1.50.1
	google.golang.org/protobuf v1.28.1
	gorm.io/driver/postgres v1.4.5
	gorm.io/gorm v1.24.1
)

require (
	github.com/beeker1121/goque v2.1.0+incompatible // indirect
	github.com/fsnotify/fsnotify v1.4.9 // indirect
	github.com/go-ole/go-ole v1.2.6 // indirect
	github.com/golang/protobuf v1.5.2 // indirect
	github.com/golang/snappy v0.0.4 // indirect
	github.com/jackc/chunkreader/v2 v2.0.1 // indirect
	github.com/jackc/pgconn v1.13.0 // indirect
	github.com/jackc/pgio v1.0.0 // indirect
	github.com/jackc/pgpassfile v1.0.0 // indirect
	github.com/jackc/pgproto3/v2 v2.3.1 // indirect
	github.com/jackc/pgservicefile v0.0.0-20200714003250-2b9c44734f2b // indirect
	github.com/jackc/pgtype v1.12.0 // indirect
	github.com/jackc/pgx/v4 v4.17.2 // indirect
	github.com/jinzhu/inflection v1.0.0 // indirect
	github.com/jinzhu/now v1.1.4 // indirect
	github.com/mitchellh/copystructure v1.2.0 // indirect
	github.com/mitchellh/mapstructure v1.5.0 // indirect
	github.com/mitchellh/reflectwalk v1.0.2 // indirect
	github.com/pelletier/go-toml v1.7.0 // indirect
	github.com/power-devops/perfstat v0.0.0-20210106213030-5aafc221ea8c // indirect
	github.com/shirou/gopsutil/v3 v3.22.3 // indirect
	github.com/syndtr/goleveldb v1.0.0 // indirect
	github.com/yusufpapurcu/wmi v1.2.2 // indirect
	go.uber.org/atomic v1.9.0 // indirect
	go.uber.org/multierr v1.6.0 // indirect
	golang.org/x/crypto v0.0.0-20220926161630-eccd6366d1be // indirect
	golang.org/x/net v0.0.0-20221002022538-bcab6841153b // indirect
	golang.org/x/sys v0.1.0 // indirect
	golang.org/x/text v0.3.7 // indirect
	google.golang.org/genproto v0.0.0-20210602131652-f16073e35f0c // indirect
)
