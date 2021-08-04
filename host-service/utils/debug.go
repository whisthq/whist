package utils // import "github.com/fractal/fractal/host-service/utils"

import (
	"fmt"
	"log"
	"os"
	"runtime/pprof"
	"sync"
	"time"
)

// WaitWithDebugPrints calls `Wait()` on the provided `*sync.WaitGroup`, but
// surrounded by some instrumentation to print debugging information with
// period `timeout`. The debugging information printed is a human-readable
// stack trace of all goroutines, to aid in finding any that are completely
// stuck.
func WaitWithDebugPrints(wg *sync.WaitGroup, timeout time.Duration, debugLevel int) {
	// Closing keepPrinting will stop the debug printing.
	keepPrinting := make(chan interface{})

	printDebug := func() {
		now := time.Now()
		fmt.Printf("------- BEGIN GOROUTINE DEBUG OUTPUT FOR %v -------\n", now)
		pprof.Lookup("goroutine").WriteTo(os.Stdout, debugLevel)
		fmt.Printf("------- END GOROUTINE DEBUG OUTPUT FOR %v ------\n", now)
	}

	log.Printf("Waiting for goroutines to finish...")

	go func() {
		for {
			timer := time.NewTimer(timeout)

			select {
			case <-keepPrinting:
				return

			case <-timer.C:
				printDebug()
				timer = time.NewTimer(timeout)
			}
		}
	}()

	wg.Wait()

	log.Printf("Done waiting for goroutines to finish!")
	close(keepPrinting)
}
