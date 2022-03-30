package payments

import (
	"context"
	"strconv"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/stripe/stripe-go/v72"
	"github.com/stripe/stripe-go/v72/checkout/session"
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

type StripeClient struct {
	restricted          string
	monthlyPriceInCents int64
}

func (sc *StripeClient) Initialize() error {
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

	restricted, ok := configs["STRIPE_RESTRICTED"]
	if !ok {
		return utils.MakeError("Could not find key STRIPE_RESTRICTED in configurations map.")
	}

	price, ok := configs["MONTHLY_PRICE_IN_CENTS"]
	if !ok {
		return utils.MakeError("Could not find key MONTHLY_PRICE_IN_CENTS in configurations map.")
	}

	monthlyPrice, err := strconv.ParseInt(price, 10, 64)
	if err != nil {
		return utils.MakeError("failed to parse monthly price. Err: %v", err)
	}

	// Assign to StripeClient
	stripe.Key = secret
	sc.restricted = restricted
	sc.monthlyPriceInCents = monthlyPrice

	return nil
}

func CreateCheckoutSession(customer string) (*stripe.CheckoutSession, error) {
	priceID := "asdasdasd"

	params := &stripe.CheckoutSessionParams{
		// Customer: ,
		LineItems: []*stripe.CheckoutSessionLineItemParams{
			{
				Price: stripe.String(priceID),
			},
			{
				Quantity: aws.Int64(1),
			},
		},
		SubscriptionData: &stripe.CheckoutSessionSubscriptionDataParams{
			TrialPeriodDays: stripe.Int64(14),
		},
		Mode:       stripe.String(string(stripe.CheckoutSessionModeSubscription)),
		SuccessURL: stripe.String("http://localhost/callback/payment?success=true"),
		CancelURL:  stripe.String("http://localhost/callback/payment?success=false"),
	}

	s, err := session.New(params)
	if err != nil {
		return nil, utils.MakeError("failed to create checkout session. Err: %v", err)
	}

	return s, nil
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
