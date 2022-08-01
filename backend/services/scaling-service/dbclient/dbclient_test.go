package dbclient

import (
	"context"
	"os"
	"sort"
	"testing"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

var (
	testImages            []subscriptions.WhistImage
	testInstances         []subscriptions.WhistInstance
	testMandelboxes       []subscriptions.WhistMandelbox
	testDBClient          *DBClient
	mockImagesClient      *mockImagesGraphQLClient
	mockInstancesClient   *mockInstancesGraphQLClient
	mockMandelboxesClient *mockMandelboxesGraphQLClient
)

func setup() {
	testDBClient = &DBClient{}
	mockImagesClient = &mockImagesGraphQLClient{}
	mockInstancesClient = &mockInstancesGraphQLClient{}
	mockMandelboxesClient = &mockMandelboxesGraphQLClient{}

	testInstances = []subscriptions.WhistInstance{
		{
			ID:                graphql.String(utils.PlaceholderTestUUID().String()),
			Provider:          graphql.String("AWS"),
			Region:            graphql.String("us-east-1"),
			ImageID:           graphql.String("test_image_id"),
			ClientSHA:         graphql.String("test_sha"),
			IPAddress:         "0.0.0.0",
			Type:              graphql.String("g4dn.xlarge"),
			RemainingCapacity: graphql.Int(2),
			Status:            graphql.String("ACTIVE"),
		},
		{
			ID:                graphql.String(utils.PlaceholderTestUUID().String()),
			Provider:          graphql.String("AWS"),
			Region:            graphql.String("us-east-1"),
			ImageID:           graphql.String("test_image_id"),
			ClientSHA:         graphql.String("test_sha"),
			IPAddress:         "0.0.0.0",
			Type:              graphql.String("g4dn.xlarge"),
			RemainingCapacity: graphql.Int(2),
			Status:            graphql.String("ACTIVE"),
		},
		{
			ID:                graphql.String(utils.PlaceholderTestUUID().String()),
			Provider:          graphql.String("AWS"),
			Region:            graphql.String("us-east-1"),
			ImageID:           graphql.String("test_image_id"),
			ClientSHA:         graphql.String("test_sha"),
			IPAddress:         "0.0.0.0",
			Type:              graphql.String("g4dn.xlarge"),
			RemainingCapacity: graphql.Int(2),
			Status:            graphql.String("ACTIVE"),
		},
	}
}

func TestMain(m *testing.M) {
	setup()
	code := m.Run()
	os.Exit(code)
}

// Types for mocking GraphQL queries

// Mock type for image queries

type mockImagesGraphQLClient struct{}

func (mc *mockImagesGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockImagesGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch query := query.(type) {
	case *struct {
		WhistImages []subscriptions.WhistImage `graphql:"whist_images(where: {provider: {_eq: $provider}, _and: {region: {_eq: $region}}}, order_by: {updated_at: desc})"`
	}:
		for _, image := range testImages {
			if image.Provider == vars["provider"] &&
				image.Region == vars["region"] {
				query.WhistImages = append(query.WhistImages, image)
			}
		}

	default:
	}

	return nil
}

func (mc *mockImagesGraphQLClient) Mutate(ctx context.Context, mutation subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch mutation := mutation.(type) {
	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_images(objects: $objects)"`
	}:
		for _, image := range vars["objects"].([]whist_images_insert_input) {
			testImages = append(testImages, subscriptions.WhistImage{
				Provider:  image.Provider,
				Region:    image.Region,
				ImageID:   image.ImageID,
				ClientSHA: image.ClientSHA,
				UpdatedAt: image.UpdatedAt,
			})
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testImages))

	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_images(where: {region: {_eq: $region}, _and: {provider: {_eq: $provider}}}, _set: {client_sha: $client_sha, image_id: $image_id, provider: $provider, region: $region, updated_at: $updated_at})"`
	}:
		for i := 0; i < len(testImages); i++ {
			if testImages[i].ImageID == vars["image_id"] {
				testImages[i].ImageID = vars["image_id"].(graphql.String)
				testImages[i].Region = vars["region"].(graphql.String)
				testImages[i].Provider = vars["provider"].(graphql.String)
				testImages[i].ClientSHA = vars["client_sha"].(graphql.String)
				testImages[i].UpdatedAt = vars["updated_at"].(timestamptz).Time
			}
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testImages))

	default:
	}
	return nil
}

// Mock type for instance queries

type mockInstancesGraphQLClient struct{}

func (mc *mockInstancesGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockInstancesGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	// While this type switch might seem unnecessary and repetitive, it is necessary to mock the
	// functionality of the Hasura client. Otherwise we would need to reimplement the logic for
	// creating queries and reponding to them in a dynamic way, which will introduce more complex code.
	switch query := query.(type) {

	case *struct {
		WhistInstances []subscriptions.WhistInstance `graphql:"whist_instances(where: {id: {_eq: $id}, _and: {status: {_eq: $status}}})"`
	}:
		for _, instance := range testInstances {
			if instance.ID == vars["id"] &&
				instance.Status == vars["status"] {
				query.WhistInstances = append(query.WhistInstances, instance)
			}
		}

	case *struct {
		WhistInstances []subscriptions.WhistInstance `graphql:"whist_instances(where: {id: {_eq: $id}})"`
	}:
		for _, instance := range testInstances {
			if instance.ID == vars["id"] {
				query.WhistInstances = append(query.WhistInstances, instance)
			}
		}

	case *struct {
		WhistInstances []subscriptions.WhistInstance `graphql:"whist_instances(where: {status: {_eq: $status}})"`
	}:
		for _, instance := range testInstances {
			if instance.Status == vars["status"] {
				query.WhistInstances = append(query.WhistInstances, instance)
			}
		}

	case *struct {
		WhistInstances []subscriptions.WhistInstance `graphql:"whist_instances(where: {region: {_eq: $region}, _and: {status: {_eq: $status}, _and: {remaining_capacity: {_gt: 0}}}}, order_by: {remaining_capacity: asc})"`
	}:

		sortedInstances := testInstances
		sort.Slice(sortedInstances, func(i, j int) bool {
			return sortedInstances[i].RemainingCapacity > sortedInstances[j].RemainingCapacity
		})

		for _, instance := range sortedInstances {
			if instance.Region == vars["region"] &&
				instance.Status == vars["status"] &&
				instance.RemainingCapacity > 0 {
				query.WhistInstances = append(query.WhistInstances, instance)
			}
		}

	case *struct {
		WhistInstances []subscriptions.WhistInstance `graphql:"whist_instances(where: {image_id: {_eq: $image_id}})"`
	}:
		for _, instance := range testInstances {
			if instance.ImageID == vars["image_id"] {
				query.WhistInstances = append(query.WhistInstances, instance)
			}
		}

	case *struct {
		WhistInstances []subscriptions.WhistInstance `graphql:"whist_instances(where: {status: {_eq: $status} _and: {region: {_eq: $region}}})"`
	}:
		for _, instance := range testInstances {
			if instance.Status == vars["status"] &&
				instance.Region == vars["region"] {
				query.WhistInstances = append(query.WhistInstances, instance)
			}
		}

	default:
	}

	return nil
}

func (mc *mockInstancesGraphQLClient) Mutate(ctx context.Context, mutation subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch mutation := mutation.(type) {
	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_instances(objects: $objects)"`
	}:
		for _, instance := range vars["objects"].([]whist_instances_insert_input) {
			testInstances = append(testInstances, subscriptions.WhistInstance{
				ID:                instance.ID,
				Provider:          instance.Provider,
				Region:            instance.Region,
				ImageID:           instance.ImageID,
				ClientSHA:         instance.ClientSHA,
				IPAddress:         instance.IPAddress,
				Type:              instance.Type,
				RemainingCapacity: instance.RemainingCapacity,
				Status:            instance.Status,
			})
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testInstances))

	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_instances(where: {id: {_eq: $id}}, _set: $changes)"`
	}:
		for i := 0; i < len(testInstances); i++ {
			if testInstances[i].ID == vars["id"] {
				testInstances[i].ID = vars["changes"].(whist_instances_set_input).ID
				testInstances[i].Region = vars["changes"].(whist_instances_set_input).Region
				testInstances[i].Provider = vars["changes"].(whist_instances_set_input).Provider
				testInstances[i].ImageID = vars["changes"].(whist_instances_set_input).ImageID
				testInstances[i].ClientSHA = vars["changes"].(whist_instances_set_input).ClientSHA
				testInstances[i].IPAddress = vars["changes"].(whist_instances_set_input).IPAddress
				testInstances[i].Type = vars["changes"].(whist_instances_set_input).Type
				testInstances[i].RemainingCapacity = vars["changes"].(whist_instances_set_input).RemainingCapacity
				testInstances[i].Status = vars["changes"].(whist_instances_set_input).Status
			}
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testInstances))

	case *struct {
		MandelboxesMutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"delete_whist_mandelboxes(where: {instance_id: {_eq: $id}})"`
		InstancesMutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"delete_whist_instances(where: {id: {_eq: $id}})"`
	}:
		var deleted int
		for i := 0; i < len(testInstances); i++ {
			if testInstances[i].ID == vars["id"] {
				testInstances = append(testInstances[:i], testInstances[i+1:]...)
				deleted++
			}
		}
		mutation.InstancesMutationResponse.AffectedRows = graphql.Int(deleted)

	default:
	}
	return nil
}

// Mock type for image queries

type mockMandelboxesGraphQLClient struct{}

func (mc *mockMandelboxesGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockMandelboxesGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch query := query.(type) {
	case *struct {
		WhistMandelboxes []subscriptions.WhistMandelbox `graphql:"whist_mandelboxes(where: {instance_id: {_eq: $instance_id}, _and: {status: {_eq: $status}}})"`
	}:
		for _, mandelbox := range testMandelboxes {
			if mandelbox.InstanceID == vars["instance_id"] &&
				mandelbox.Status == vars["status"] {
				query.WhistMandelboxes = append(query.WhistMandelboxes, mandelbox)
			}
		}

	default:
	}

	return nil
}

func (mc *mockMandelboxesGraphQLClient) Mutate(ctx context.Context, mutation subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch mutation := mutation.(type) {
	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_mandelboxes(where: {id: {_eq: $id}}, _set: $changes)"`
	}:
		var updated int
		for i := 0; i < len(testMandelboxes); i++ {
			if testMandelboxes[i].ID == vars["id"] {
				testMandelboxes[i].ID = vars["changes"].(whist_mandelboxes_set_input).ID
				testMandelboxes[i].App = vars["changes"].(whist_mandelboxes_set_input).App
				testMandelboxes[i].InstanceID = vars["changes"].(whist_mandelboxes_set_input).InstanceID
				testMandelboxes[i].SessionID = vars["changes"].(whist_mandelboxes_set_input).SessionID
				testMandelboxes[i].Status = vars["changes"].(whist_mandelboxes_set_input).Status
				testMandelboxes[i].UserID = vars["changes"].(whist_mandelboxes_set_input).UserID
				updated++
			}
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(updated)

	default:
	}
	return nil
}
