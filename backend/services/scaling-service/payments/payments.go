package payments

import (
	"context"
	"strconv"

	"github.com/stripe/stripe-go/v72"
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

// PaymentsClient is a wrapper struct that will call our
// own Stripe client and allow testing. Packages that handle
// payments should only use this struct.
type PaymentsClient struct {
	stripeClient WhistStripeClient
}

// Initialize will pull all necessary configurations from the database
// and set the StripeClient fields with the values extracted from
// the access token.
func (whistPayments *PaymentsClient) Initialize(customerID string, subscriptionStatus string, configGraphqlClient subscriptions.WhistGraphQLClient) error {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Get configurations depending on environment
	var (
		configs map[string]string
		err     error
	)

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

	// Create our own Stripe client
	whistPayments.stripeClient = &StripeClient{}

	// Dynamically set the Stripe key depending on environment
	if metadata.IsLocalEnv() {
		// Use the restriced Strip key if on localdev or testing
		restricedKey, ok := configs["STRIPE_RESTRICTED"]
		if !ok {
			return utils.MakeError("Could not find key STRIPE_RESTRICTED in configurations map.")
		}

		stripe.Key = restricedKey
		whistPayments.stripeClient.(*StripeClient).key = restricedKey
	} else {
		stripe.Key = secret
		whistPayments.stripeClient.(*StripeClient).key = secret
	}

	whistPayments.stripeClient.(*StripeClient).monthlyPriceInCents = monthlyPrice
	whistPayments.stripeClient.(*StripeClient).customerID = customerID
	whistPayments.stripeClient.(*StripeClient).subscriptionStatus = subscriptionStatus

	return nil
}

// CreateSession is the method exposed to other packages for getting a Stripe Session URL.
// It should only be called after `Initialize` has been run first. It decides what kind of
// Stripe Session to return based on the customer's subscription status.
func (whistPayments *PaymentsClient) CreateSession() (string, error) {
	var (
		sessionUrl string
		err        error
	)
	subscriptionStatus := whistPayments.stripeClient.(*StripeClient).subscriptionStatus
	if subscriptionStatus == "active" || subscriptionStatus == "trialing" {
		// If the authenticated user already has a Whist subscription in a non-terminal state
		// (one of `active` or `trialing`), create a Stripe billing portal that the customer
		// can use to manage their subscription and billing information.
		sessionUrl, err = whistPayments.stripeClient.createBillingPortal()
		if err != nil {
			return "", utils.MakeError("error creating Stripe billing portal. Err: %v", err)
		}
	} else {
		// If the authenticated user has a Whist subscriptions in a terminal states (not `active` or `trialing`),
		// create a Stripe checkout session that the customer can use to enroll in a new Whist subscription.
		sessionUrl, err = whistPayments.stripeClient.createCheckoutSession()
		if err != nil {
			return "", utils.MakeError("error creating Stripe checkout session. Err: %v", err)
		}
	}

	return sessionUrl, nil
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
