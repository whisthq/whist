package configutils // import "github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"

import (
	"bytes"
	"context"
	"crypto/md5"
	"encoding/hex"
	"path"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/aws/arn"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	"github.com/aws/aws-sdk-go-v2/service/s3control"
	"github.com/aws/aws-sdk-go-v2/service/sts"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// NewS3Client returns a new AWS S3 client.
func NewS3Client(region string) (*s3.Client, error) {
	cfg, err := config.LoadDefaultConfig(context.Background())
	if err != nil {
		return nil, utils.MakeError("failed to load aws config: %v", err)
	}
	return s3.NewFromConfig(cfg, func(o *s3.Options) {
		o.Region = region
	}), nil
}

// NewS3ControlClient returns a new AWS S3 Control client.
func NewS3ControlClient(region string) (*s3control.Client, error) {
	cfg, err := config.LoadDefaultConfig(context.Background())
	if err != nil {
		return nil, utils.MakeError("failed to load aws config: %v", err)
	}
	return s3control.NewFromConfig(cfg, func(o *s3control.Options) {
		o.Region = region
	}), nil
}

// NewS3ControlClient returns a new AWS STS client.
func NewSTSClient(region string) (*sts.Client, error) {
	cfg, err := config.LoadDefaultConfig(context.Background())
	if err != nil {
		return nil, utils.MakeError("failed to load aws config: %v", err)
	}
	return sts.NewFromConfig(cfg, func(o *sts.Options) {
		o.Region = region
	}), nil
}

// GetAccountNumber will get the AWS account number.
func GetAccountNumber() (string, error) {
	client, err := NewSTSClient("us-east-1")
	if err != nil {
		return "", utils.MakeError("couldn't create s3control client: %s", err)
	}

	output, err := client.GetCallerIdentity(context.Background(), &sts.GetCallerIdentityInput{})
	if err != nil {
		return "", utils.MakeError("failed to get caller identity: %s", err)
	}

	return aws.ToString(output.Account), nil
}

// GetAccessPoint returns the ARN of the multi-region access point used to
// access encrypted user configs.
var GetAccessPoint func() string = func(unmemoized func() string) func() string {
	// This nested function syntax is used to memoize the result of the first call
	// to GetAccessPoint() and cache the result for all future calls.

	var isCached = false
	var cache string

	return func() string {
		if isCached {
			return cache
		}
		cache = unmemoized()
		isCached = true
		return cache
	}
}(func() string {
	// For some reason, AWS routes all requests to manage the multi-region
	// access point to Oregon, so the region needs to be set to us-west-2.
	client, err := NewS3ControlClient("us-west-2")
	if err != nil {
		logger.Errorf("couldn't create s3control client: %s", err)
		return ""
	}

	account, err := GetAccountNumber()
	if err != nil {
		logger.Errorf("couldn't get account number: %s", err)
		return ""
	}

	var input *s3control.GetMultiRegionAccessPointInput
	env := metadata.GetAppEnvironmentLowercase()
	if env == string(metadata.EnvDev) || env == string(metadata.EnvStaging) || env == string(metadata.EnvProd) {
		// Return the appropiate mmulti-region access point depending on current environment
		input = &s3control.GetMultiRegionAccessPointInput{
			AccountId: aws.String(account),
			Name:      aws.String(utils.Sprintf("whist-user-configs-access-point-%s", env)),
		}
	} else {
		// Default to dev
		input = &s3control.GetMultiRegionAccessPointInput{
			AccountId: aws.String(account),
			Name:      aws.String("whist-user-configs-access-point-dev"),
		}
	}
	output, err := client.GetMultiRegionAccessPoint(context.Background(), input)
	if err != nil {
		logger.Errorf("failed to get access point: %s", err)
		return ""
	}

	accessPointARN := arn.ARN{
		Partition: "aws",
		AccountID: account,
		Service:   "s3",
		Resource:  utils.Sprintf("accesspoint/%s", aws.ToString(output.AccessPoint.Alias)),
	}

	return accessPointARN.String()
})

// GetHeadObject returns the head object of the given bucket and key.
func GetHeadObject(client *s3.Client, bucket, key string) (*s3.HeadObjectOutput, error) {
	return client.HeadObject(context.Background(), &s3.HeadObjectInput{
		Bucket: aws.String(bucket),
		Key:    aws.String(key),
	})
}

// DownloadObjectToBuffer downloads the given object to the given buffer.
// Returns the number of bytes downloaded and an error if any.
func DownloadObjectToBuffer(client *s3.Client, bucket, key string, buffer *manager.WriteAtBuffer) (int64, error) {
	downloader := manager.NewDownloader(client)
	return downloader.Download(context.Background(), buffer, &s3.GetObjectInput{
		Bucket: aws.String(bucket),
		Key:    aws.String(key),
	})
}

// UploadFileToBucket uploads the given file to the given bucket and key.
func UploadFileToBucket(client *s3.Client, bucket, key string, data []byte) (*manager.UploadOutput, error) {
	uploader := manager.NewUploader(client)
	dataBuffer := bytes.NewBuffer(data)
	return uploader.Upload(context.Background(), &s3.PutObjectInput{
		Bucket: aws.String(bucket),
		Key:    aws.String(key),
		Body:   dataBuffer,
		Metadata: map[string]string{
			"md5": GetMD5Hash(data),
		},
	})
}

// GetMd5Hash returns the MD5 hash of the given data as a hex string.
func GetMD5Hash(data []byte) string {
	hash := md5.Sum(data)
	return hex.EncodeToString(hash[:])
}

// UpdateMostRecentToken updates the user's most recently used config token
// file in S3 with the provided token.
func UpdateMostRecentToken(client *s3.Client, user types.UserID, token string) error {
	recentTokenPath := path.Join("last-used-tokens", string(user))
	_, err := UploadFileToBucket(client, GetAccessPoint(), recentTokenPath, []byte(token))
	if err != nil {
		return utils.MakeError("failed to update most recent token: %v", err)
	}
	return nil
}

// GetMostRecentToken returns the most recently used token for the given user.
func GetMostRecentToken(client *s3.Client, user types.UserID) (string, error) {
	recentTokenPath := path.Join("last-used-tokens", string(user))
	dataBuffer := manager.NewWriteAtBuffer([]byte{})
	_, err := DownloadObjectToBuffer(client, GetAccessPoint(), recentTokenPath, dataBuffer)
	if err != nil {
		return "", utils.MakeError("failed to get most recent token: %v", err)
	}
	return string(dataBuffer.Bytes()), nil
}
