package configutils // import "github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"

import (
	"bytes"
	"context"
	"crypto/md5"
	"encoding/hex"
	"strings"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	s3types "github.com/aws/aws-sdk-go-v2/service/s3/types"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
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

// GetConfigBucket returns name of the S3 bucket that contains the encrypted user configs.
func GetConfigBucket() string {
	env := metadata.GetAppEnvironmentLowercase()
	if env == string(metadata.EnvDev) || env == string(metadata.EnvStaging) || env == string(metadata.EnvProd) {
		// Return the appropiate bucket depending on current environment
		return utils.Sprintf("whist-user-app-configs-%s", metadata.GetAppEnvironmentLowercase())
	} else {
		// Default to dev
		return utils.Sprintf("whist-user-app-configs-dev")
	}
}

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

// GetMostRecentKey scans the provided S3 bucket for keys beginning with the
// provided prefix and ending with the provided suffix. It returns the key
// whose value was most recently modified.
func GetMostRecentMatchingKey(client *s3.Client, bucket, prefix, suffix string) (*s3types.Object, error) {
	var curMatch *s3types.Object = nil

	// We deal with the case where there are more than 1000 subkeys for the
	// user's app config with pagination. God forbid this ever happen, but we
	// gotta write resilient software.
	for paginator := s3.NewListObjectsV2Paginator(client, &s3.ListObjectsV2Input{
		Bucket: aws.String(bucket),
		Prefix: aws.String(prefix),
	}); paginator.HasMorePages(); {
		output, err := paginator.NextPage(context.Background())
		if err != nil {
			return curMatch, err
		}

		for i, v := range output.Contents {
			if (curMatch == nil || curMatch.LastModified.Before(*v.LastModified)) && strings.HasSuffix(*v.Key, suffix) {
				// DO NOT use `&v` instead of `&output.Contents[i]`, since Go reuses
				// memory addresses for loop variables (see the link below). I
				// re-learned this the hard way.
				// https://www.evanjones.ca/go-gotcha-loop-variables.html
				curMatch = &output.Contents[i]
			}
		}
	}

	return curMatch, nil
}

// GetMd5Hash returns the MD5 hash of the given data as a hex string.
func GetMD5Hash(data []byte) string {
	hash := md5.Sum(data)
	return hex.EncodeToString(hash[:])
}
