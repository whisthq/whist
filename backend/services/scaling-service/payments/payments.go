package payments

import (
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/utils"
)

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
