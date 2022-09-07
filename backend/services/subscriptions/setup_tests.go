package subscriptions

import (
	"context"
	"log"
	"sync"

	"net/http"
	"time"

	"math/rand"

	"github.com/google/uuid"
	"github.com/graph-gophers/graphql-go"
	"github.com/graph-gophers/graphql-go/relay"
	"github.com/graph-gophers/graphql-transport-ws/graphqlws"
	hasura "github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/utils"
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

// Subscription server and helper functions

// The following code has been pulled from the Hasura GraphQL client library to ease setting up
// a subscription server for testing. This code is used to test our own subscriptions library.
// Note: Due to limitations on the graphql server libraries, we are unable to test our own
// subscriptions (instances, mandelboxes) without having to implement a GraphQL server.
// As such, we test using the provided "HelloSaid" subscription against our own client implementation.
// https://github.com/hasura/go-graphql-client/blob/master/subscription_test.go

const schema = `
schema {
	subscription: Subscription
	mutation: Mutation
	query: Query
}
type Query {
	hello: String!
}
type Subscription {
	helloSaid(): HelloSaidEvent!
}
type Mutation {
	sayHello(msg: String!): HelloSaidEvent!
}
type HelloSaidEvent {
	id: String!
	msg: String!
}
`

type resolver struct {
	helloSaidEvents     chan *helloSaidEvent
	helloSaidSubscriber chan *helloSaidSubscriber
}

func (r *resolver) Hello() string {
	return "Hello world!"
}

func (r *resolver) SayHello(args struct{ Msg string }) *helloSaidEvent {
	e := &helloSaidEvent{msg: args.Msg, id: randomID()}
	go func() {
		select {
		case r.helloSaidEvents <- e:
		case <-time.After(1 * time.Second):
		}
	}()
	return e
}

type helloSaidSubscriber struct {
	stop   <-chan struct{}
	events chan<- *helloSaidEvent
}

func (r *resolver) broadcastHelloSaid() {
	subscribers := map[string]*helloSaidSubscriber{}
	unsubscribe := make(chan string)

	// NOTE: subscribing and sending events are at odds.
	for {
		select {
		case id := <-unsubscribe:
			delete(subscribers, id)
		case s := <-r.helloSaidSubscriber:
			subscribers[randomID()] = s
		case e := <-r.helloSaidEvents:
			for id, s := range subscribers {
				go func(id string, s *helloSaidSubscriber) {
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

func (r *resolver) HelloSaid(ctx context.Context) <-chan *helloSaidEvent {
	c := make(chan *helloSaidEvent)
	// NOTE: this could take a while
	r.helloSaidSubscriber <- &helloSaidSubscriber{events: c, stop: ctx.Done()}

	return c
}

type helloSaidEvent struct {
	id  string
	msg string
}

func (r *helloSaidEvent) Msg() string {
	return r.msg
}

func (r *helloSaidEvent) ID() string {
	return r.id
}

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
	opts := graphql.SchemaOpt(graphql.UseFieldResolvers())
	s, err := graphql.ParseSchema(schema, newResolver(), opts)
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

func newResolver() *resolver {
	r := &resolver{
		helloSaidEvents:     make(chan *helloSaidEvent),
		helloSaidSubscriber: make(chan *helloSaidSubscriber),
	}

	go r.broadcastHelloSaid()

	return r
}

func randomID() string {
	var letter = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")

	b := make([]rune, 16)
	for i := range b {
		b[i] = letter[rand.Intn(len(letter))]
	}
	return string(b)
}

func runServerAndSubscribe(ctx context.Context, server *http.Server, subscriptionClient *SubscriptionClient, sub GraphQLQuery, events chan SubscriptionEvent) error {
	// Start subscription server
	go func() {
		if err := server.ListenAndServe(); err != nil {
			log.Println(err)
		}
	}()

	subscriptionClient.Hasura.WithLog(log.Print)

	var result map[string]map[string]string
	_, err := subscriptionClient.Subscribe(sub, nil, result, func(se SubscriptionEvent, m map[string]interface{}) bool { return true }, events)
	if err != nil {
		return utils.MakeError("got error: %v, want: nil", err)
	}

	return nil
}

// triggerSubscription will send a mutation to the database so that
// the server detects a change and sends the subscription to all
// subscribers.
func triggerSubscription(client *GraphQLClient, msg string) error {
	// wait until the subscription client connects to the server
	time.Sleep(2 * time.Second)

	// call a mutation request to send message to the subscription
	var q struct {
		SayHello struct {
			ID  hasura.String
			Msg hasura.String
		} `graphql:"sayHello(msg: $msg)"`
	}
	variables := map[string]interface{}{
		"msg": hasura.String(msg),
	}
	return client.Mutate(context.Background(), &q, variables)
}
