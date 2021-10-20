import ndt7 from "@m-lab/ndt7"
import events from "events"

// Maximum amount of time we want the download test to take in seconds
const MILLISECONDS = 1000
const TEST_DURATION_SECONDS = 5 * MILLISECONDS

// Config taken from @m-lab/ndt7 repo example
const config = { userAcceptedDataPolicy: true }

// The download test emits results over a period of time; create variables
// to track those results
let iterations = 0
let results = {
  latency: 0,
  jitter: 0,
  downloadMbps: 0,
  progress: 0,
}

// Handles download measurements
const handleDownloadMeasurements = (results: any, iterations: number) => {
  return (measurement: any) => {
    if (measurement.Source === "client") {
      results.downloadMbps = measurement.Data.MeanClientMbps
    } else {
      iterations += 1
      results.progress =
        (measurement.Data.TCPInfo.ElapsedTime /
          (TEST_DURATION_SECONDS * 1000)) *
        100
      const latencyInMs = measurement.Data.TCPInfo.RTT / 1000
      const jitterInMs = measurement.Data.TCPInfo.RTTVar / 1000
      results.latency =
        (results.latency * (iterations - 1) + latencyInMs) / iterations
      results.jitter =
        (results.jitter * (iterations - 1) + jitterInMs) / iterations
    }

    // If enough time has passed but we aren't done with the test, emit what we have so far
    if (results.progress >= 100) {
      testComplete("success")
    }
  }
}

const networkAnalysisEvent = new events.EventEmitter()
const testComplete = (status: string) => {
  networkAnalysisEvent.emit(status, results)
}

// Callbacks to pass into download speed test
const callbacks = {
  downloadMeasurement: handleDownloadMeasurements(results, iterations),
  downloadComplete: () => {
    testComplete("success")
  },
  error: (err: { message: string }) => {
    console.error(err.message)
    testComplete("error")
  },
}

const networkTest = () => {
  const urlPromise = ndt7.discoverServerURLs(config, callbacks)
  ndt7.downloadTest(config, callbacks, urlPromise)

  setTimeout(() => {
    testComplete("success")
  }, TEST_DURATION_SECONDS)
}

export { networkTest }
