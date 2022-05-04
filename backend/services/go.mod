module github.com/whisthq/whist/backend/services

go 1.17

require (
	github.com/MicahParks/keyfunc v1.1.0
	github.com/NVIDIA/go-nvml v0.11.6-0
	github.com/aws/aws-sdk-go-v2 v1.16.3
	github.com/aws/aws-sdk-go-v2/config v1.15.4
	github.com/aws/aws-sdk-go-v2/feature/s3/manager v1.11.8
	github.com/aws/aws-sdk-go-v2/service/ec2 v1.38.0
	github.com/aws/aws-sdk-go-v2/service/s3 v1.26.8
	github.com/aws/smithy-go v1.11.2
	github.com/bgentry/heroku-go v0.0.0-20150810151148-ee4032d686ae
	github.com/docker/docker v20.10.14+incompatible
	github.com/docker/go-connections v0.4.0
	github.com/docker/go-units v0.4.0
	github.com/fsnotify/fsnotify v1.5.4
	github.com/getsentry/sentry-go v0.13.0
	github.com/go-co-op/gocron v1.13.0
	github.com/golang-jwt/jwt/v4 v4.4.1
	github.com/google/go-cmp v0.5.8
	github.com/google/uuid v1.3.0
	github.com/gorilla/websocket v1.5.0
	github.com/hasura/go-graphql-client v0.6.5
	github.com/jackc/pgconn v1.12.0
	github.com/jackc/pgtype v1.11.0
	github.com/jackc/pgx/v4 v4.16.0
	github.com/lithammer/shortuuid/v3 v3.0.7
	github.com/logzio/logzio-go v1.0.4
	github.com/opencontainers/image-spec v1.0.2
	github.com/pierrec/lz4/v4 v4.1.14
	github.com/shirou/gopsutil/v3 v3.22.4
	golang.org/x/crypto v0.0.0-20220427172511-eb4f295cb31f
	golang.org/x/sync v0.0.0-20210220032951-036812b2e83c
	golang.org/x/time v0.0.0-20220411224347-583f2d630306
)

require (
	github.com/Microsoft/go-winio v0.5.2 // indirect
	github.com/aws/aws-sdk-go-v2/aws/protocol/eventstream v1.4.1 // indirect
	github.com/aws/aws-sdk-go-v2/credentials v1.12.0 // indirect
	github.com/aws/aws-sdk-go-v2/feature/ec2/imds v1.12.4 // indirect
	github.com/aws/aws-sdk-go-v2/internal/configsources v1.1.10 // indirect
	github.com/aws/aws-sdk-go-v2/internal/endpoints/v2 v2.4.4 // indirect
	github.com/aws/aws-sdk-go-v2/internal/ini v1.3.11 // indirect
	github.com/aws/aws-sdk-go-v2/internal/v4a v1.0.1 // indirect
	github.com/aws/aws-sdk-go-v2/service/internal/accept-encoding v1.9.1 // indirect
	github.com/aws/aws-sdk-go-v2/service/internal/checksum v1.1.5 // indirect
	github.com/aws/aws-sdk-go-v2/service/internal/presigned-url v1.9.4 // indirect
	github.com/aws/aws-sdk-go-v2/service/internal/s3shared v1.13.4 // indirect
	github.com/aws/aws-sdk-go-v2/service/sso v1.11.4 // indirect
	github.com/aws/aws-sdk-go-v2/service/sts v1.16.4 // indirect
	github.com/beeker1121/goque v2.1.0+incompatible // indirect
	github.com/bgentry/testnet v0.0.0-20131107221340-05450cdcf16c // indirect
	github.com/containerd/containerd v1.6.2 // indirect
	github.com/docker/distribution v2.8.1+incompatible // indirect
	github.com/go-ole/go-ole v1.2.6 // indirect
	github.com/gogo/protobuf v1.3.2 // indirect
	github.com/golang/protobuf v1.5.2 // indirect
	github.com/golang/snappy v0.0.4 // indirect
	github.com/gorilla/mux v1.8.0 // indirect
	github.com/hashicorp/go-version v1.4.0
	github.com/jackc/chunkreader/v2 v2.0.1 // indirect
	github.com/jackc/pgio v1.0.0 // indirect
	github.com/jackc/pgpassfile v1.0.0 // indirect
	github.com/jackc/pgproto3/v2 v2.3.0 // indirect
	github.com/jackc/pgservicefile v0.0.0-20200714003250-2b9c44734f2b // indirect
	github.com/jackc/puddle v1.2.1 // indirect
	github.com/jmespath/go-jmespath v0.4.0 // indirect
	github.com/klauspost/compress v1.15.1 // indirect
	github.com/lufia/plan9stats v0.0.0-20220326011226-f1430873d8db // indirect
	github.com/moby/term v0.0.0-20210619224110-3f7ff695adc6 // indirect
	github.com/morikuni/aec v1.0.0 // indirect
	github.com/onsi/ginkgo v1.16.4 // indirect
	github.com/onsi/gomega v1.15.0 // indirect
	github.com/opencontainers/go-digest v1.0.0 // indirect
	github.com/pborman/uuid v1.2.1 // indirect
	github.com/pkg/errors v0.9.1 // indirect
	github.com/power-devops/perfstat v0.0.0-20220216144756-c35f1ee13d7c // indirect
	github.com/robfig/cron/v3 v3.0.1 // indirect
	github.com/sirupsen/logrus v1.8.1 // indirect
	github.com/stripe/stripe-go/v72 v72.104.0
	github.com/syndtr/goleveldb v1.0.0 // indirect
	github.com/tklauser/go-sysconf v0.3.10 // indirect
	github.com/tklauser/numcpus v0.4.0 // indirect
	github.com/yusufpapurcu/wmi v1.2.2 // indirect
	go.uber.org/atomic v1.9.0 // indirect
	golang.org/x/net v0.0.0-20220325170049-de3da57026de // indirect
	golang.org/x/sys v0.0.0-20220412211240-33da011f77ad // indirect
	golang.org/x/text v0.3.7 // indirect
	google.golang.org/genproto v0.0.0-20220328180837-c47567c462d1 // indirect
	google.golang.org/grpc v1.45.0 // indirect
	google.golang.org/protobuf v1.28.0 // indirect
	gotest.tools/v3 v3.1.0 // indirect
	nhooyr.io/websocket v1.8.7 // indirect
)

replace github.com/gin-gonic/gin => github.com/gin-gonic/gin v1.7.7
