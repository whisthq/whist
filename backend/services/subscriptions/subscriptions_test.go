package subscriptions

import (
	"reflect"
	"sync"
	"testing"

	"context"
	"math/rand"
	"net/http"
	"time"

	"github.com/google/uuid"
	"github.com/graph-gophers/graphql-go"
	"github.com/graph-gophers/graphql-go/relay"
	"github.com/graph-gophers/graphql-transport-ws/graphqlws"
	hasura "github.com/hasura/go-graphql-client"
)

// mockWhistClient is a struct that mocks a real Hasura client for testing.
type mockWhistClient struct {
	Params          HasuraParams
	Subscriptions   []HasuraSubscription
	SubscriptionIDs []string
}

func (cl *mockWhistClient) Initialize(useConfigDB bool) error {
	return nil
}

func (cl *mockWhistClient) GetSubscriptions() []HasuraSubscription {
	return cl.Subscriptions
}

func (cl *mockWhistClient) SetSubscriptions(subscriptions []HasuraSubscription) {
	cl.Subscriptions = subscriptions
}

func (cl *mockWhistClient) GetSubscriptionIDs() []string {
	return cl.SubscriptionIDs
}

func (cl *mockWhistClient) SetSubscriptionsIDs(subscriptionIDs []string) {
	cl.SubscriptionIDs = subscriptionIDs
}

func (cl *mockWhistClient) GetParams() HasuraParams {
	return cl.Params
}

func (cl *mockWhistClient) SetParams(params HasuraParams) {
	cl.Params = params
}

const schema = `
schema {
	subscription: Subscription
	mutation: Mutation
	query: Query
}
type Query {
	InstanceQuery: [whist_instances!]
}
type whist_instances {
	id: String!
	provider: String!
	region: String!
	image_id: String!
	client_sha: String!
	ip_addr: String!
	instance_type: String!
	remaining_capacity: String!
	status: String!
	created_at: String!
	updated_at: String!
}

type Subscription {
	SubscriptionFired(): InstanceEvent!
}
type Mutation {
	SendSubscription(msg: String!): InstanceEvent!
}
type InstanceEvent {
	id: String!
	whist_instances: [whist_instances!]
}
`

func subscription_setupClients() (*GraphQLClient, *SubscriptionClient) {
	endpoint := "http://localhost:4679/graphql"

	client := hasura.NewClient(endpoint, &http.Client{Transport: http.DefaultTransport})

	subscriptionClient := hasura.NewSubscriptionClient(endpoint).
		WithConnectionParams(map[string]interface{}{
			"headers": map[string]string{
				"foo": "bar",
			},
		})

	whistSubscriptionClient := &SubscriptionClient{}
	whistSubscriptionClient.Hasura = subscriptionClient

	whistGraphQLClient := &GraphQLClient{}
	whistGraphQLClient.Hasura = client

	return whistGraphQLClient, whistSubscriptionClient
}

func subscription_setupServer() *http.Server {

	// init graphQL schema
	s, err := graphql.ParseSchema(schema, newResolver())
	if err != nil {
		panic(err)
	}

	// graphQL handler
	mux := http.NewServeMux()
	graphQLHandler := graphqlws.NewHandlerFunc(s, &relay.Handler{Schema: s})
	mux.HandleFunc("/graphql", graphQLHandler)
	server := &http.Server{Addr: ":4679", Handler: mux}

	return server
}

type resolver struct {
	instanceEvents     chan *instanceEvent
	instanceSubscriber chan *instanceSubscriber
}

func newResolver() *resolver {
	r := &resolver{
		instanceEvents:     make(chan *instanceEvent),
		instanceSubscriber: make(chan *instanceSubscriber),
	}

	go r.broadcastHelloSaid()

	return r
}

func (r *resolver) InstanceQuery() *[]Instance {
	return &[]Instance{
		{},
	}
}

func (r *resolver) SendSubscription(args struct{ Msg string }) *instanceEvent {
	e := &instanceEvent{}
	e.instances = []Instance{
		{ID: uuid.NewString(), ImageID: "test_image_id"},
	}
	go func() {
		select {
		case r.instanceEvents <- e:
		case <-time.After(1 * time.Second):
		}
	}()
	return e
}

type instanceSubscriber struct {
	stop   <-chan struct{}
	events chan<- *instanceEvent
}

func (r *resolver) broadcastHelloSaid() {
	subscribers := map[string]*instanceSubscriber{}
	unsubscribe := make(chan string)

	// NOTE: subscribing and sending events are at odds.
	for {
		select {
		case id := <-unsubscribe:
			delete(subscribers, id)
		case s := <-r.instanceSubscriber:
			subscribers[randomID()] = s
		case e := <-r.instanceEvents:
			for id, s := range subscribers {
				go func(id string, s *instanceSubscriber) {
					select {
					case <-s.stop:
						unsubscribe <- id
						return
					default:
					}

					select {
					case <-s.stop:
						unsubscribe <- id
					case s.events <- e:
					case <-time.After(time.Second):
					}
				}(id, s)
			}
		}
	}
}

func (r *resolver) SubscriptionFired(ctx context.Context) <-chan *instanceEvent {
	c := make(chan *instanceEvent)
	// NOTE: this could take a while
	r.instanceSubscriber <- &instanceSubscriber{events: c, stop: ctx.Done()}

	return c
}

type instanceEvent struct {
	id        string
	instances []Instance
}

func (r *instanceEvent) Instances() []Instance {
	return r.instances
}

func (r *instanceEvent) ID() string {
	return r.id
}

func randomID() string {
	var letter = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")

	b := make([]rune, 16)
	for i := range b {
		b[i] = letter[rand.Intn(len(letter))]
	}
	return string(b)
}

// Subscribe mocks creating subscriptions. For testing, only returned predefined test events.
func (cl *mockWhistClient) Subscribe(query GraphQLQuery, variables map[string]interface{}, result SubscriptionEvent,
	conditionFn Handlerfn, subscriptionEvents chan SubscriptionEvent) (string, error) {

	// Create fake instance event
	testInstanceEvent := InstanceEvent{Instances: []Instance{{
		ID:     variables["id"].(string),
		Status: variables["status"].(string),
	}}}

	// Create fake mandelbox event
	testMandelboxEvent := MandelboxEvent{Mandelboxes: []Mandelbox{{
		InstanceID: variables["instance_id"].(string),
		Status:     "ALLOCATED",
	}}}

	// Send fake event through channel depending on the result type received
	switch result.(type) {

	case InstanceEvent:
		if conditionFn(testInstanceEvent, variables) {
			subscriptionEvents <- testInstanceEvent
		}
	case MandelboxEvent:
		if conditionFn(testMandelboxEvent, variables) {
			subscriptionEvents <- testMandelboxEvent
		}
	default:

	}

	return uuid.NewString(), nil
}

func (cl *mockWhistClient) Run(goroutinetracker *sync.WaitGroup) {}

func (cl *mockWhistClient) Close() error {
	return nil
}
func TestInstanceStatusHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"id":     hasura.String("test-instance-id"),
		"status": hasura.String("DRAINING"),
	}

	// Create different tests for the instance status handler,
	// verify if it returns the appropiate response
	var instanceTests = []struct {
		testName string
		event    InstanceEvent
		want     bool
	}{
		{"Empty event", InstanceEvent{Instances: []Instance{}}, false},
		{"Wrong status event", InstanceEvent{
			Instances: []Instance{{ID: ""}},
		}, false},
		{"Correct status event", InstanceEvent{
			Instances: []Instance{
				{ID: "test-instance-id", Status: "DRAINING"},
			},
		}, true},
	}

	for _, tt := range instanceTests {
		testname := tt.testName
		t.Run(testname, func(t *testing.T) {
			got := InstanceStatusHandler(tt.event, variables)

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}
func TestMandelboxAllocatedHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"instance_id": hasura.String("test-instance-id"),
		"status":      hasura.String("ALLOCATED"),
	}

	// Create different tests for the mandelbox allocated handler,
	// verify if it returns the appropiate response
	var mandelboxTests = []struct {
		testName string
		event    MandelboxEvent
		want     bool
	}{
		{"Empty event", MandelboxEvent{Mandelboxes: []Mandelbox{}}, false},
		{"Wrong instance id event", MandelboxEvent{
			Mandelboxes: []Mandelbox{
				{InstanceID: "test-instance-id-2", Status: "EXITED"},
			},
		}, false},
		{"Correct status event", MandelboxEvent{
			Mandelboxes: []Mandelbox{
				{InstanceID: "test-instance-id", Status: "ALLOCATED"},
			},
		}, true},
	}

	for _, tt := range mandelboxTests {
		testname := tt.testName
		t.Run(testname, func(t *testing.T) {
			got := MandelboxAllocatedHandler(tt.event, variables)

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}

func TestSetupHostSubscriptions(t *testing.T) {
	instanceID := "test-instance-id"
	whistClient := &mockWhistClient{}

	// Create the host service specific subscriptions
	SetupHostSubscriptions(instanceID, whistClient)

	if whistClient.Subscriptions == nil {
		t.Errorf("Got nil subscriptions")
	}

	if len(whistClient.Subscriptions) != 2 {
		t.Errorf("Expected subscriptions lenght to be 2, got: %v", len(whistClient.Subscriptions))
	}

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"id": hasura.String(instanceID),
	}

	// Verify that the "variables" maps are deep equal for the first subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables = map[string]interface{}{
		"instance_id": hasura.String(instanceID),
		"status":      hasura.String("ALLOCATED"),
	}

	// Verify that the "variables" maps are deep equal for the second subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[1].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[1].Variables, variables)
	}
}

func TestSetupScalingSubscriptions(t *testing.T) {
	whistClient := &mockWhistClient{}

	// Create the scaling service specific subscriptions
	SetupScalingSubscriptions(whistClient)

	if whistClient.Subscriptions == nil {
		t.Errorf("Got nil subscriptions")
	}

	if len(whistClient.Subscriptions) != 1 {
		t.Errorf("Expected subscriptions lenght to be 1, got: %v", len(whistClient.Subscriptions))
	}

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"status": hasura.String("DRAINING"),
	}

	// Verify that the "variables" maps are deep equal for the first subscription

	if !reflect.DeepEqual(variables, whistClient.Subscriptions[0].Variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}
}

// TODO: Add test to subscriptions start function.
