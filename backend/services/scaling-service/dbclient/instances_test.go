package dbclient

import (
	"context"
	"reflect"
	"testing"

	"github.com/google/uuid"
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

func TestQueryInstance(t *testing.T) {
	var tests = []struct {
		name     string
		err      bool
		expected subscriptions.Instance
	}{
		{"Successful query", false, subscriptions.Instance{
			ID:                utils.PlaceholderTestUUID().String(),
			Provider:          "AWS",
			Region:            "us-east-1",
			ImageID:           "test_image_id",
			ClientSHA:         "test_sha",
			IPAddress:         "0.0.0.0",
			Type:              "g4dn.xlarge",
			RemainingCapacity: 2,
			Status:            "ACTIVE",
		}},
		{"Not found", true, subscriptions.Instance{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testInstances[0].ID = graphql.String(uuid.NewString())
			defer func() {
				testInstances[0].ID = graphql.String(utils.PlaceholderTestUUID().String())
			}()

			res, err := testDBClient.QueryInstance(context.Background(), mockInstancesClient, tt.expected.ID)
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 && !tt.err {
				t.Fatalf("failed to query for instance, got empty result")
			} else if tt.err {
				return
			}

			if ok := reflect.DeepEqual(tt.expected, res[0]); !ok {
				t.Fatalf("incorrect instance returned from query, expected %v, got %v", tt.expected, res[0])
			}
		})
	}
}

func TestQueryInstanceWithCapacity(t *testing.T) {
	var tests = []struct {
		name     string
		region   string
		err      bool
		expected []subscriptions.Instance
	}{
		{"Successful query", "us-east-1", false, []subscriptions.Instance{
			{
				ID:                utils.PlaceholderTestUUID().String(),
				Provider:          "AWS",
				Region:            "us-east-1",
				ImageID:           "test_image_id",
				ClientSHA:         "test_sha",
				IPAddress:         "0.0.0.0",
				Type:              "g4dn.xlarge",
				RemainingCapacity: 2,
				Status:            "ACTIVE",
			},
			{
				ID:                utils.PlaceholderTestUUID().String(),
				Provider:          "AWS",
				Region:            "us-east-1",
				ImageID:           "test_image_id",
				ClientSHA:         "test_sha",
				IPAddress:         "0.0.0.0",
				Type:              "g4dn.xlarge",
				RemainingCapacity: 1,
				Status:            "ACTIVE",
			},
		}},
		{"Not found", "us-west-1", true, nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Setup the testInstances slice to values that are
			// useful for this test. Return to the initial value
			// once finished so other tests can use the slice.
			testInstances[0].RemainingCapacity = graphql.Int(0)
			testInstances[1].RemainingCapacity = graphql.Int(1)
			testInstances[2].RemainingCapacity = graphql.Int(2)
			defer func() {
				testInstances[0].RemainingCapacity = graphql.Int(2)
				testInstances[1].RemainingCapacity = graphql.Int(2)
				testInstances[2].RemainingCapacity = graphql.Int(2)
			}()

			res, err := testDBClient.QueryInstanceWithCapacity(context.Background(), mockInstancesClient, tt.region)
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 && !tt.err {
				t.Fatalf("failed to query for instance, got empty result")
			}

			if ok := reflect.DeepEqual(res, tt.expected); !ok {
				t.Fatalf("incorrect instance returned from query, expected %v, got %v", tt.expected, res)
			}
		})
	}
}

func TestQueryInstancesByStatusOnRegion(t *testing.T) {
	var tests = []struct {
		name           string
		status, region string
		err            bool
		expected       []subscriptions.Instance
	}{
		{"Successful query", "DRAINING", "us-east-1", false, []subscriptions.Instance{
			{
				ID:                utils.PlaceholderTestUUID().String(),
				Provider:          "AWS",
				Region:            "us-east-1",
				ImageID:           "test_image_id",
				ClientSHA:         "test_sha",
				IPAddress:         "0.0.0.0",
				Type:              "g4dn.xlarge",
				RemainingCapacity: 2,
				Status:            "DRAINING",
			},
		}},
		{"Not found", "PRE_CONNECTION", "us-east-1", true, nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testInstances[1].Status = "DRAINING"
			defer func() {
				testInstances[1].Status = "ACTIVE"
			}()

			res, err := testDBClient.QueryInstancesByStatusOnRegion(context.Background(), mockInstancesClient, tt.status, tt.region)
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 && !tt.err {
				t.Fatalf("failed to query for instance, got empty result")
			}

			if ok := reflect.DeepEqual(res, tt.expected); !ok {
				t.Fatalf("incorrect instance returned from query, expected %v, got %v", tt.expected, res)
			}
		})
	}
}

func TestQueryInstancesByImage(t *testing.T) {
	var tests = []struct {
		name     string
		image    string
		err      bool
		expected []subscriptions.Instance
	}{
		{"Successful query", "test_image_id", false, []subscriptions.Instance{
			{
				ID:                utils.PlaceholderTestUUID().String(),
				Provider:          "AWS",
				Region:            "us-east-1",
				ImageID:           "test_image_id",
				ClientSHA:         "test_sha",
				IPAddress:         "0.0.0.0",
				Type:              "g4dn.xlarge",
				RemainingCapacity: 2,
				Status:            "ACTIVE",
			},
		}},
		{"Not found", "fake_image_id", true, nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testInstances[0].ImageID = "old_image_id"
			testInstances[1].ImageID = "test_image_id"
			testInstances[2].ImageID = "new_image_id"
			defer func() {
				testInstances[0].ImageID = "test_image_id"
				testInstances[1].ImageID = "test_image_id"
				testInstances[2].ImageID = "test_image_id"
			}()

			res, err := testDBClient.QueryInstancesByImage(context.Background(), mockInstancesClient, tt.image)
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 && !tt.err {
				t.Fatalf("failed to query for instance, got empty result")
			}

			if ok := reflect.DeepEqual(res, tt.expected); !ok {
				t.Fatalf("incorrect instance returned from query, expected %v, got %v", tt.expected, res)
			}
		})
	}
}

func TestInsertInstances(t *testing.T) {
	var tests = []struct {
		name     string
		expected []subscriptions.WhistInstance
	}{
		{"Insert one", []subscriptions.WhistInstance{
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("AWS"),
				Region:            graphql.String("us-west-1"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
		}},
		{"Insert multiple", []subscriptions.WhistInstance{
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("AWS"),
				Region:            graphql.String("us-east-2"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("GC"),
				Region:            graphql.String("us-west1"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
		}},
		{"Insert zero", []subscriptions.WhistInstance{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testInstances = []subscriptions.WhistInstance{}

			rows, err := testDBClient.InsertInstances(context.Background(), mockInstancesClient, subscriptions.ToInstances(tt.expected))
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(tt.expected) {
				t.Fatalf("incorrect number of rows inserted, expected %d rows, inserted %d", len(tt.expected), rows)
			}

			if ok := reflect.DeepEqual(testInstances, tt.expected); !ok {
				t.Fatalf("incorrect instances inserted, expected %v, got %v", tt.expected, testInstances)
			}
		})
	}
}

func TestUpdateInstance(t *testing.T) {
	var tests = []struct {
		name     string
		update   []subscriptions.WhistInstance
		expected []subscriptions.WhistInstance
	}{
		{"Update existing", []subscriptions.WhistInstance{
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("AWS"),
				Region:            graphql.String("us-east-1"),
				ImageID:           graphql.String("new_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
		}, []subscriptions.WhistInstance{
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("AWS"),
				Region:            graphql.String("us-east-1"),
				ImageID:           graphql.String("new_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
		}},
		{"Update non existing", []subscriptions.WhistInstance{
			{
				ID:                graphql.String(uuid.NewString()),
				Provider:          graphql.String("Azure"),
				Region:            graphql.String("us-west1"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
		}, []subscriptions.WhistInstance{
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("GC"),
				Region:            graphql.String("us-west1"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
		}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {

			testInstances = []subscriptions.WhistInstance{
				{
					ID:                graphql.String(utils.PlaceholderTestUUID().String()),
					Provider:          graphql.String("GC"),
					Region:            graphql.String("us-west1"),
					ImageID:           graphql.String("test_image_id"),
					ClientSHA:         graphql.String("test_sha"),
					IPAddress:         "0.0.0.0",
					Type:              graphql.String("g4dn.xlarge"),
					RemainingCapacity: graphql.Int(2),
					Status:            graphql.String("ACTIVE"),
				},
			}

			update := subscriptions.ToInstances(tt.update)
			rows, err := testDBClient.UpdateInstance(context.Background(), mockInstancesClient, update[0])
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(tt.update) {
				t.Fatalf("incorrect number of rows updated, expected %d rows, updated %d", len(tt.update), rows)
			}

			if ok := reflect.DeepEqual(testInstances, tt.expected); !ok {
				t.Fatalf("incorrect instances updated, expected %v, got %v", tt.expected, testInstances)
			}
		})
	}
}

func TestDeleteInstance(t *testing.T) {
	var tests = []struct {
		name     string
		delete   string
		expected []subscriptions.WhistInstance
	}{
		{"Delete existing", utils.PlaceholderTestUUID().String(), []subscriptions.WhistInstance{
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("AWS"),
				Region:            graphql.String("us-east-1"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("DRAINING"),
			},
		}},
		{"Delete non existing", uuid.NewString(), []subscriptions.WhistInstance{
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("AWS"),
				Region:            graphql.String("us-east-1"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("DRAINING"),
			},
			{
				ID:                graphql.String(utils.PlaceholderTestUUID().String()),
				Provider:          graphql.String("GC"),
				Region:            graphql.String("us-west1"),
				ImageID:           graphql.String("test_image_id"),
				ClientSHA:         graphql.String("test_sha"),
				IPAddress:         "0.0.0.0",
				Type:              graphql.String("g4dn.xlarge"),
				RemainingCapacity: graphql.Int(2),
				Status:            graphql.String("ACTIVE"),
			},
		}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {

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
					Status:            graphql.String("DRAINING"),
				},
				{
					ID:                graphql.String(utils.PlaceholderWarmupUUID().String()),
					Provider:          graphql.String("GC"),
					Region:            graphql.String("us-west1"),
					ImageID:           graphql.String("test_image_id"),
					ClientSHA:         graphql.String("test_sha"),
					IPAddress:         "0.0.0.0",
					Type:              graphql.String("g4dn.xlarge"),
					RemainingCapacity: graphql.Int(2),
					Status:            graphql.String("ACTIVE"),
				},
			}

			rows, err := testDBClient.DeleteInstance(context.Background(), mockInstancesClient, tt.delete)
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(tt.expected) {
				t.Fatalf("incorrect number of rows deleted, expected %d rows, deleted %d", len(tt.expected), rows)
			}

			if ok := reflect.DeepEqual(testInstances, tt.expected); !ok {
				t.Fatalf("incorrect instances updated, expected %v, got %v", tt.expected, testInstances)
			}
		})
	}
}
