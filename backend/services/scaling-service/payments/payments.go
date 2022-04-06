package payments

import (
	"context"
	"strconv"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/stripe/stripe-go/v72"
	billingPortal "github.com/stripe/stripe-go/v72/billingportal/session"
	checkout "github.com/stripe/stripe-go/v72/checkout/session"
	"github.com/stripe/stripe-go/v72/price"
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// PaymentsHandler is an abstraction of the methods used for
// processing payments for a client.
type PaymentsHandler interface {
	Initialize() error
	CreateSession() (string, error)
	createCheckoutSession() (string, error)
	createBillingPortal() (string, error)
}

// StripeClient implements PaymentsHandler, and interacts directly
// with the official Stripe client.
type StripeClient struct {
	customerID          string
	subscriptionStatus  string
	monthlyPriceInCents int64
}

// Initialize will pull all necessary configurations from the database
// and set the StripeClient fields with the values extracted from
// the access token.
func (sc *StripeClient) Initialize(customerID string, subscriptionStatus string) error {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Start GraphQL client for getting configuration from the config db
	useConfigDB := true
	configGraphqlClient := &subscriptions.GraphQLClient{}
	err := configGraphqlClient.Initialize(useConfigDB)
	if err != nil {
		logger.Errorf("Failed to start config GraphQL client. Error: %v", err)
	}

	// Get configurations depending on environment
	var configs map[string]string

	switch metadata.GetAppEnvironmentLowercase() {
	case string(metadata.EnvDev):
		configs, err = dbclient.GetDevConfigs(ctx, configGraphqlClient)
	case string(metadata.EnvStaging):
		configs, err = dbclient.GetStagingConfigs(ctx, configGraphqlClient)
	case string(metadata.EnvProd):
		configs, err = dbclient.GetProdConfigs(ctx, configGraphqlClient)
	default:
		configs, err = dbclient.GetDevConfigs(ctx, configGraphqlClient)
	}

	if err != nil {
		// Err is already wrapped here
		return err
	}

	// Verify that all necessary configurations are present
	secret, ok := configs["STRIPE_SECRET"]
	if !ok {
		return utils.MakeError("Could not find key STRIPE_SECRET in configurations map.")
	}

	price, ok := configs["MONTHLY_PRICE_IN_CENTS"]
	if !ok {
		return utils.MakeError("Could not find key MONTHLY_PRICE_IN_CENTS in configurations map.")
	}

	monthlyPrice, err := strconv.ParseInt(price, 10, 64)
	if err != nil {
		return utils.MakeError("failed to parse monthly price. Err: %v", err)
	}

	// Initialize StripeClient fields
	stripe.Key = secret
	sc.monthlyPriceInCents = monthlyPrice

	sc.customerID = customerID
	sc.subscriptionStatus = subscriptionStatus

	return nil
}

// createCheckoutSession creates a Stripe checkout session for the current customer.
// It applies the desired price from the database, and if it doesn't exist in Stripe,
//  creates it.
func (sc *StripeClient) createCheckoutSession() (string, error) {
	priceList := price.List(&stripe.PriceListParams{
		Active: stripe.Bool(true),
	})

	var priceID string

	// Try to find a Stripe Price with the desired monthly price.
	for priceList.Next() {
		currentPrice := priceList.Current().(*stripe.Price)
		if currentPrice.UnitAmount == sc.monthlyPriceInCents {
			break
		}
	}
	if priceList.Err() != nil {
		return "", utils.MakeError("failed to read price list from Stripe. Err: %v", priceList.Err())
	}

	// If a Stripe Price was not found, create one.
	if priceID != "" {
		logger.Infof("Found price of %v in Stripe.", sc.monthlyPriceInCents)
	} else {
		//Create new price
		newPrice, err := createPrice(sc.monthlyPriceInCents, "Whist", "month")
		if err != nil {
			return "", utils.MakeError("failed to create new Strip price. Err: %v", err)
		}
		priceID = newPrice.ID
	}

	// Set how many subscriptions we want started, and what number of
	// days we offer as a trial period.
	subscriptionQuantity := 1
	trialDays := int64(14)

	params := &stripe.CheckoutSessionParams{
		Customer: stripe.String(sc.customerID),
		LineItems: []*stripe.CheckoutSessionLineItemParams{
			{
				Price:    stripe.String(priceID),
				Quantity: aws.Int64(int64(subscriptionQuantity)),
			},
		},
		SubscriptionData: &stripe.CheckoutSessionSubscriptionDataParams{
			TrialPeriodDays: stripe.Int64(trialDays),
		},
		Mode:       stripe.String(string(stripe.CheckoutSessionModeSubscription)),
		SuccessURL: stripe.String("http://localhost/callback/payment?success=true"),
		CancelURL:  stripe.String("http://localhost/callback/payment?success=false"),
	}

	s, err := checkout.New(params)
	if err != nil {
		return "", utils.MakeError("failed to create checkout session. Err: %v", err)
	}

	return s.URL, nil
}

// createBillingPortal creates a Stripe billing portal for the current customer.
func (sc *StripeClient) createBillingPortal() (string, error) {
	s, err := billingPortal.New(&stripe.BillingPortalSessionParams{
		Customer:  stripe.String(sc.customerID),
		ReturnURL: stripe.String("http://localhost/callback/payment"),
	})

	return s.URL, err
}

// CreateSession is the method exposed to other packages for getting a Stripe Session URL.
// It should only be called after `Initialize` has been run first. It decides what kind of
// Stripe Session to return based on the customer's subscription status.
func (sc *StripeClient) CreateSession() (string, error) {
	var (
		sessionUrl string
		err        error
	)

	if sc.subscriptionStatus == "active" || sc.subscriptionStatus == "trialing" {
		// If the authenticated user already has a Whist subscription in a non-terminal state
		// (one of `active` or `trialing`), create a Stripe billing portal that the customer
		// can use to manage their subscription and billing information.
		sessionUrl, err = sc.createBillingPortal()
		if err != nil {
			return "", utils.MakeError("error creating Stripe billing portal. Err: %v", err)
		}
	} else {
		// If the authenticated user has a Whist subscriptions in a terminal states (not `active` or `trialing`),
		// create a Stripe checkout session that the customer can use to enroll in a new Whist subscription.
		sessionUrl, err = sc.createCheckoutSession()
		if err != nil {
			return "", utils.MakeError("error creating Stripe checkout session. Err: %v", err)
		}
	}

	return sessionUrl, nil
}

// createPrice is a helper function that creates a new Price object in Stripe
// with the desired price (in cents), for the product `name` in the given interval.
func createPrice(cents int64, name string, interval string) (*stripe.Price, error) {
	return price.New(&stripe.PriceParams{
		UnitAmount: stripe.Int64(cents),
		Currency:   stripe.String("usd"),
		ProductData: &stripe.PriceProductDataParams{
			Name: stripe.String(name),
		},
		Recurring: &stripe.PriceRecurringParams{
			Interval: stripe.String(interval),
		},
	})
}

// VerifyPayment takes an access token and extracts the claims within it.
// Once it has obtained the claims, it checks if the customer has an active
// Stripe subscription using the subscription status custom claim.
func VerifyPayment(accessToken string) (bool, error) {
	claims, err := auth.ParseToken(accessToken)
	if err != nil {
		return false, utils.MakeError("failed to parse access token. Err: %v", err)
	}

	status := claims.SubscriptionStatus
	if status == "" {
		return false, utils.MakeError("subscription status claim is empty.")
	}

	paymentValid := status == "active" || status == "trialing"

	return paymentValid, nil
}
