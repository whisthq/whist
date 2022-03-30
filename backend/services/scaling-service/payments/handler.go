package payments

import (
	"net/http"

	"github.com/stripe/stripe-go/v72"
)

type StripeClient struct {
}

func (sc *StripeClient) Initialize() {
	stripe.Key = "sk_test_4eC39HqLyjWDarjtT1zdp7dc"
}

func createCheckoutSession(w http.ResponseWriter, r *http.Request) {
}
