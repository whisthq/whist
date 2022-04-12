package payments

import (
	"context"
	"os"
	"reflect"
	"strconv"
	"testing"

	"github.com/hasura/go-graphql-client"
	"github.com/stripe/stripe-go/v72"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

var (
	testGraphQLCLient subscriptions.WhistGraphQLClient
	testPayments      *PaymentsClient
	testConfigMap     map[string]string
)

func setup() {
	testGraphQLCLient = &mockConfigGraphQLClient{}
	testPayments = &PaymentsClient{}
}

func TestMain(m *testing.M) {
	setup()
	code := m.Run()
	os.Exit(code)
}

type mockConfigGraphQLClient struct {
}

func (gc *mockConfigGraphQLClient) Initialize(bool) error {
	return nil
}

func (gc *mockConfigGraphQLClient) Query(context context.Context, query subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	// Use the values set by tests
	configs := []struct {
		Key   graphql.String `graphql:"key"`
		Value graphql.String `graphql:"value"`
	}{
		{
			Key:   "STRIPE_SECRET",
			Value: graphql.String(testConfigMap["STRIPE_SECRET"]),
		},
		{
			Key:   "STRIPE_RESTRICTED",
			Value: graphql.String(testConfigMap["STRIPE_RESTRICTED"]),
		},
		{
			Key:   "MONTHLY_PRICE_IN_CENTS",
			Value: graphql.String(testConfigMap["MONTHLY_PRICE_IN_CENTS"]),
		},
	}

	// Set the query value according to environment. This switch is necessary so we can
	// modify the `WhistConfigs` field of the query.
	switch metadata.GetAppEnvironment() {
	case metadata.EnvDev:
		query.(*struct {
			subscriptions.WhistConfigs "graphql:\"dev\""
		}).WhistConfigs = configs
	case metadata.EnvStaging:
		query.(*struct {
			subscriptions.WhistConfigs "graphql:\"staging\""
		}).WhistConfigs = configs
	case metadata.EnvProd:
		query.(*struct {
			subscriptions.WhistConfigs "graphql:\"prod\""
		}).WhistConfigs = configs
	}
	return nil
}

func (gc *mockConfigGraphQLClient) Mutate(context.Context, subscriptions.GraphQLQuery, map[string]interface{}) error {
	return nil
}

type mockStripeClient struct {
	key                 string
	customerID          string
	subscriptionStatus  string
	monthlyPriceInCents int64
}

func (sc *mockStripeClient) configure(secret string, restrictedSecret string, customerID string, subscriptionStatus string, monthlyPriceInCents int64) {
	// Dynamically set the Stripe key depending on environment
	if metadata.IsLocalEnv() {
		sc.key = restrictedSecret
	} else {
		sc.key = secret
	}

	sc.customerID = customerID
	sc.subscriptionStatus = subscriptionStatus
	sc.monthlyPriceInCents = monthlyPriceInCents
}

func (sc *mockStripeClient) getSubscription() (*stripe.Subscription, error) {
	return nil, nil
}

func (sc *mockStripeClient) getSubscriptionStatus() string {
	return sc.subscriptionStatus
}

func (sc *mockStripeClient) findPrice() (string, error) {
	return "", nil
}

func (sc *mockStripeClient) isNewUser() bool {
	return false
}

func (sc *mockStripeClient) createCheckoutSession(withTrialPeriod bool) (string, error) {
	return "https://test-checkout-session.url", nil
}

func (sc *mockStripeClient) createBillingPortal() (string, error) {
	return "https://test-billing-portal.url", nil
}

func (sc *mockStripeClient) createPrice(cents int64, name string, interval string) (*stripe.Price, error) {
	return &stripe.Price{}, nil
}

// TestInitialize will test the StipeClient's `Initialize` function, and verify
// that it sets the fields correctly with the values received from the config database.
// For this test we mock the database interactions.
func TestInitialize(t *testing.T) {
	tests := []struct {
		env                 string
		customer            string
		subscriptionStatus  string
		stripeSecret        string
		monthlyPriceInCents string
		stripeRestricted    string
	}{
		{
			env:                 "dev",
			customer:            "test_customer_dev",
			subscriptionStatus:  "expired",
			stripeSecret:        "sk_test_key_dev",
			monthlyPriceInCents: "25000",
			stripeRestricted:    "rk_test_key_dev",
		},
		{
			env:                 "staging",
			customer:            "test_customer_staging",
			subscriptionStatus:  "trialing",
			stripeSecret:        "sk_test_key_staging",
			monthlyPriceInCents: "26000",
			stripeRestricted:    "rk_test_key_staging",
		},
		{
			env:                 "prod",
			customer:            "test_customer_prod",
			subscriptionStatus:  "active",
			stripeSecret:        "sk_test_key_prod",
			monthlyPriceInCents: "27000",
			stripeRestricted:    "rk_test_key_prod",
		},
	}

	for _, tt := range tests {
		t.Run(tt.env, func(t *testing.T) {

			// Override environments with the ones on each test
			metadata.GetAppEnvironment = func() metadata.AppEnvironment {
				return metadata.AppEnvironment(tt.env)
			}

			// Set the test configuration that will mock a database query
			testConfigMap = map[string]string{
				"STRIPE_SECRET":          tt.stripeSecret,
				"STRIPE_RESTRICTED":      tt.stripeRestricted,
				"MONTHLY_PRICE_IN_CENTS": tt.monthlyPriceInCents,
			}

			// Try to initialize a test StripeClient
			err := testPayments.Initialize(tt.customer, tt.subscriptionStatus, testGraphQLCLient, &mockStripeClient{})
			if err != nil {
				t.Fatalf("Failed to Initialize StripeClient. Err: %v", err)
			}

			wantedPrice, err := strconv.ParseInt(tt.monthlyPriceInCents, 10, 64)
			if err != nil {
				t.Errorf("Failed to parse monthly price in cents. Err: %v", err)
			}

			// Set the secret or restricted key depending on the environment.
			// The environment is overriden so the `metadata` package will use
			// the environment from the test.
			var wantSecret string
			if metadata.IsLocalEnv() {
				wantSecret = tt.stripeRestricted
			} else {
				wantSecret = tt.stripeSecret
			}

			wantClient := &mockStripeClient{
				key:                 wantSecret,
				customerID:          tt.customer,
				subscriptionStatus:  tt.subscriptionStatus,
				monthlyPriceInCents: wantedPrice,
			}

			// Verify if the fields inside the StripeClient were initialized correctly
			ok := reflect.DeepEqual(testPayments.stripeClient, wantClient)
			if !ok {
				t.Errorf("StripeClient was not initialized correctly. Want %v, got %v", wantClient, testPayments.stripeClient)
			}
		})
	}
}

func TestCreateSession(t *testing.T) {
	tests := []struct {
		subscriptionStatus     string
		checkoutSessionCreated bool
		billingPortalCreated   bool
	}{
		{"active", false, true},
		{"trialing", false, true},
		{"incomplete", true, false},
		{"incomplete_expired", true, false},
		{"past_due", true, false},
		{"canceled", true, false},
		{"unpaid", true, false},
	}

	for _, tt := range tests {
		t.Run(tt.subscriptionStatus, func(t *testing.T) {
			// Set subscription status to the one of the test
			testPayments.stripeClient.(*mockStripeClient).subscriptionStatus = tt.subscriptionStatus

			// Ask for a payments session
			url, err := testPayments.CreateSession()
			if err != nil {
				t.Errorf("Failed to create session. Err: %v", err)
			}

			// Checkout session got called
			if url == "https://test-checkout-session.url" &&
				!tt.checkoutSessionCreated {
				t.Errorf("Expected billing portal to be created for status %v, got checkout session instead.", tt.subscriptionStatus)
			}

			// Billing portal got called
			if url == "https://test-billing-portal.url" &&
				!tt.billingPortalCreated {
				t.Errorf("Expected checkout session to be created for status %v, got billing portal instead.", tt.subscriptionStatus)
			}
		})
	}
}
