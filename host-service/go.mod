module github.com/whisthq/whist/host-service

go 1.17

replace github.com/fractal/fractal/core-go => ../core-go/

require (
	github.com/MicahParks/keyfunc v1.0.1
	github.com/NVIDIA/go-nvml v0.11.1-0
	github.com/aws/aws-sdk-go-v2 v1.11.2
	github.com/aws/aws-sdk-go-v2/config v1.11.0
	github.com/aws/aws-sdk-go-v2/feature/s3/manager v1.7.4
	github.com/aws/aws-sdk-go-v2/service/s3 v1.21.0
	github.com/aws/smithy-go v1.9.0
	github.com/docker/docker v20.10.11+incompatible
	github.com/docker/go-connections v0.4.0
	github.com/docker/go-units v0.4.0
	github.com/fractal/fractal/core-go v0.0.0-00010101000000-000000000000
	github.com/golang-jwt/jwt/v4 v4.1.0
	github.com/google/uuid v1.3.0
	github.com/jackc/pgconn v1.10.1
	github.com/jackc/pgtype v1.9.1
	github.com/jackc/pgx/v4 v4.14.1
	github.com/opencontainers/image-spec v1.0.2
	github.com/pierrec/lz4/v4 v4.1.11
	github.com/shirou/gopsutil v3.21.11+incompatible
	golang.org/x/crypto v0.0.0-20211117183948-ae814b36b871
	golang.org/x/sync v0.0.0-20210220032951-036812b2e83c
)

require (
	github.com/Microsoft/go-winio v0.5.1 // indirect
	github.com/StackExchange/wmi v0.0.0-20190523213315-cbe66965904d // indirect
	github.com/aws/aws-sdk-go-v2/aws/protocol/eventstream v1.0.0 // indirect
	github.com/aws/aws-sdk-go-v2/credentials v1.6.4 // indirect
	github.com/aws/aws-sdk-go-v2/feature/ec2/imds v1.8.2 // indirect
	github.com/aws/aws-sdk-go-v2/internal/configsources v1.1.2 // indirect
	github.com/aws/aws-sdk-go-v2/internal/endpoints/v2 v2.0.2 // indirect
	github.com/aws/aws-sdk-go-v2/internal/ini v1.3.2 // indirect
	github.com/aws/aws-sdk-go-v2/service/ec2 v1.24.0 // indirect
	github.com/aws/aws-sdk-go-v2/service/internal/accept-encoding v1.5.0 // indirect
	github.com/aws/aws-sdk-go-v2/service/internal/presigned-url v1.5.2 // indirect
	github.com/aws/aws-sdk-go-v2/service/internal/s3shared v1.9.2 // indirect
	github.com/aws/aws-sdk-go-v2/service/sso v1.6.2 // indirect
	github.com/aws/aws-sdk-go-v2/service/sts v1.11.1 // indirect
	github.com/beeker1121/goque v2.1.0+incompatible // indirect
	github.com/bgentry/heroku-go v0.0.0-20150810151148-ee4032d686ae // indirect
	github.com/containerd/containerd v1.5.8 // indirect
	github.com/docker/distribution v2.7.1+incompatible // indirect
	github.com/fsnotify/fsnotify v1.5.1 // indirect
	github.com/getsentry/sentry-go v0.11.0 // indirect
	github.com/go-ole/go-ole v1.2.6 // indirect
	github.com/gogo/protobuf v1.3.2 // indirect
	github.com/golang/protobuf v1.5.0 // indirect
	github.com/golang/snappy v0.0.0-20180518054509-2e65f85255db // indirect
	github.com/gorilla/mux v1.8.0 // indirect
	github.com/hasura/go-graphql-client v0.5.1 // indirect
	github.com/jackc/chunkreader/v2 v2.0.1 // indirect
	github.com/jackc/pgio v1.0.0 // indirect
	github.com/jackc/pgpassfile v1.0.0 // indirect
	github.com/jackc/pgproto3/v2 v2.2.0 // indirect
	github.com/jackc/pgservicefile v0.0.0-20200714003250-2b9c44734f2b // indirect
	github.com/jackc/puddle v1.2.0 // indirect
	github.com/jmespath/go-jmespath v0.4.0 // indirect
	github.com/klauspost/compress v1.11.13 // indirect
	github.com/logzio/logzio-go v1.0.3 // indirect
	github.com/moby/term v0.0.0-20210619224110-3f7ff695adc6 // indirect
	github.com/morikuni/aec v1.0.0 // indirect
	github.com/opencontainers/go-digest v1.0.0 // indirect
	github.com/pborman/uuid v1.2.1 // indirect
	github.com/pkg/errors v0.9.1 // indirect
	github.com/shirou/gopsutil/v3 v3.21.6 // indirect
	github.com/sirupsen/logrus v1.8.1 // indirect
	github.com/syndtr/goleveldb v1.0.0 // indirect
	github.com/tklauser/go-sysconf v0.3.6 // indirect
	github.com/tklauser/numcpus v0.2.2 // indirect
	github.com/yusufpapurcu/wmi v1.2.2 // indirect
	go.uber.org/atomic v1.9.0 // indirect
	golang.org/x/net v0.0.0-20211112202133-69e39bad7dc2 // indirect
	golang.org/x/sys v0.0.0-20210630005230-0f9fa26af87c // indirect
	golang.org/x/text v0.3.6 // indirect
	google.golang.org/genproto v0.0.0-20201110150050-8816d57aaa9a // indirect
	google.golang.org/grpc v1.42.0 // indirect
	google.golang.org/protobuf v1.27.1 // indirect
	nhooyr.io/websocket v1.8.7 // indirect
)
