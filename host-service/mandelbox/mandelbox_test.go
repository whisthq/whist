package mandelbox

import (
	"context"
	"sync"
	"testing"

	"github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/utils"
)

func TestNewMandelbox(t *testing.T) {
	ctx := context.Background()
	routineTracker := sync.WaitGroup{}

	New(ctx, &routineTracker, types.MandelboxID(utils.PlaceholderTestUUID()))
}
