package payments

import (
	"time"

	"github.com/stripe/stripe-go/v72"
	billingPortal "github.com/stripe/stripe-go/v72/billingportal/session"
	checkout "github.com/stripe/stripe-go/v72/checkout/session"
	"github.com/stripe/stripe-go/v72/price"
	"github.com/stripe/stripe-go/v72/sub"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// WhistStripeClient abstracts our own Stripe Client with the necessary
// methods to process payments for users. Structs that implement it should
// encapsulate the logic that uses the official Stripe SDK.
type WhistStripeClient interface {
	configure(string, string, string, string, int64)
	getSubscription() (*stripe.Subscription, error)
	getSubscriptionStatus() string
	findPrice() (string, error)
	isNewUser() bool
	createCheckoutSession(bool) (string, error)
	createBillingPortal() (string, error)
	createPrice(cents int64, name string, interval string) (*stripe.Price, error)
}

// StripeClient interacts directly with the official
// Stripe client.
type StripeClient struct {
	key                 string // The secret key to authenticate calls to the Stripe API
	customerID          string // The Stripe ID of the customer
	subscriptionStatus  string // The subscription status ("active", "trialing", etc.)
	monthlyPriceInCents int64  // The desired price in cents for a Whist subscription
}

// configure will set the fields of the StripeClient to the received values. This method
// makes it easier to mock for testing.
func (sc *StripeClient) configure(secret string, restrictedSecret string, customerID string, subscriptionStatus string, monthlyPriceInCents int64) {
	// Dynamically set the Stripe key depending on environment
	if metadata.IsLocalEnv() || metadata.IsRunningInCI() {
		sc.key = restrictedSecret
	} else {
		sc.key = secret
	}

	stripe.Key = sc.key
	sc.customerID = customerID
	sc.subscriptionStatus = subscriptionStatus
	sc.monthlyPriceInCents = monthlyPriceInCents
}

// getSubscription will try to get the current subscription for the customer.
func (sc *StripeClient) getSubscription() (*stripe.Subscription, error) {
	subscriptionsList := sub.List(&stripe.SubscriptionListParams{
		Customer: sc.customerID,
		CurrentPeriodEndRange: &stripe.RangeQueryParams{
			GreaterThan: time.Now().Unix(),
		},
	})

	var subscription *stripe.Subscription
	for subscriptionsList.Next() {
		subscription = subscriptionsList.Subscription()
	}
	if subscriptionsList.Err() != nil || subscription == nil {
		return nil, utils.MakeError("Failed to obtain subscription for customer %v", sc.customerID)
	}

	return subscription, nil
}

// getSubscriptionStatus returns the subscription status obtained
// from the access token.
func (sc *StripeClient) getSubscriptionStatus() string {
	return sc.subscriptionStatus
}

// isNewUser is a helper fuction to determine if the user is new or not.
func (sc *StripeClient) isNewUser() bool {
	// Search if a user has any previous Whist subscription, either
	// cancelled or expired due to incomplete payment.
	endedSubscriptions := sub.List(&stripe.SubscriptionListParams{
		Customer: sc.customerID,
		Status:   "ended",
	})

	var hasEndedSubscriptions bool
	for endedSubscriptions.Next() {
		if endedSubscriptions.Subscription() != nil {
			hasEndedSubscriptions = true
		}
	}

	return !hasEndedSubscriptions
}

// findPrice will search for a Stripe price matching the desired amount and
// will return the price ID. If it fails to find it, then creates a new price
// in Stripe and returns its ID.
func (sc *StripeClient) findPrice() (string, error) {
	var priceID string

	priceList := price.List(&stripe.PriceListParams{
		Active: stripe.Bool(true),
	})

	// Try to find a Stripe Price with the desired monthly price.
	for priceList.Next() {
		currentPrice := priceList.Current().(*stripe.Price)
		if currentPrice.UnitAmount == sc.monthlyPriceInCents {
			priceID = currentPrice.ID
			break
		}
	}

	if priceList.Err() != nil || priceID == "" {
		logger.Warningf("Failed to find price in Stripe. Creating new price.")
	}

	// If a Stripe Price was not found, create one.
	if priceID != "" {
		logger.Infof("Found price of %v in Stripe.", sc.monthlyPriceInCents)
	} else {
		//Create new price
		newPrice, err := sc.createPrice(sc.monthlyPriceInCents, "Whist", "month")
		if err != nil {
			return "", utils.MakeError("failed to create new Stripe price. Err: %v", err)
		}
		priceID = newPrice.ID
	}

	return priceID, nil
}

// createCheckoutSession creates a Stripe checkout session for the current customer.
func (sc *StripeClient) createCheckoutSession(withTrialPeriod bool) (string, error) {
	// Set how many subscriptions we want started, and what number of
	// days we offer as a trial period.
	const (
		subscriptionQuantity = 1  // The number of subscriptions that will be created
		trialPeriodDays      = 14 // The number of days offered as a free trial period
	)

	// Get a Stripe price with the desired amount
	priceID, err := sc.findPrice()
	if err != nil {
		return "", utils.MakeError("failed to find or create Stripe price for amount %v. Err: %v", sc.monthlyPriceInCents, err)
	}

	// Create subscription params depending if we are offering a free trial or not
	var subscriptionParams *stripe.CheckoutSessionSubscriptionDataParams
	if withTrialPeriod {
		subscriptionParams = &stripe.CheckoutSessionSubscriptionDataParams{
			TrialPeriodDays: stripe.Int64(trialPeriodDays),
		}
	} else {
		subscriptionParams = &stripe.CheckoutSessionSubscriptionDataParams{}
	}

	params := &stripe.CheckoutSessionParams{
		Customer: stripe.String(sc.customerID),
		LineItems: []*stripe.CheckoutSessionLineItemParams{
			{
				Price:    stripe.String(priceID),
				Quantity: stripe.Int64(int64(subscriptionQuantity)),
			},
		},
		SubscriptionData: subscriptionParams,
		PaymentMethodTypes: []*string{
			stripe.String("card"),
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

// createBillingPortal creates a Stripe billing portal for the current customer to manage
// the subscription.
func (sc *StripeClient) createBillingPortal() (string, error) {
	s, err := billingPortal.New(&stripe.BillingPortalSessionParams{
		Customer:  stripe.String(sc.customerID),
		ReturnURL: stripe.String("http://localhost/callback/payment"),
	})

	return s.URL, err
}

// createPrice is a helper function that creates a new Price object in Stripe
// with the desired price (in cents), for the product `name` in the given interval.
func (sc *StripeClient) createPrice(cents int64, name string, interval string) (*stripe.Price, error) {
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
