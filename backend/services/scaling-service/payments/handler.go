package payments

type PaymentsHandler interface {
	Initialize() error
	CreateCheckoutSession(string) error
}
