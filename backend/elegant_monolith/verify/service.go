package verify

import (
	"context"
	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

type VerifyService interface {
	internal.Service
	VerifyCapacity(ctx context.Context) (err error)
	VerifyInstanceScaleDown(ctx context.Context, instanceID string) (err error)
	VerifyInstanceRemoval(ctx context.Context, instanceID string) (err error)
}

type verifyService struct {
	// db WhistDBCLient
}

func (verifyService) VerifyCapacity(ctx context.Context) (err error) {
	return nil
}

func (verifyService) VerifyInstanceScaleDown(ctx context.Context, instanceID string) (err error) {
	return nil
}

func (verifyService) VerifyInstanceRemoval(ctx context.Context, instanceID string) (err error) {
	return nil
}
