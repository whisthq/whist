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
  jitter: 0,
  downloadMbps: 0,
  progress: 0,
}
let emitted = false

// Handles download measurements
const handleDownloadMeasurements = (
  results: { jitter: number; downloadMbps: number; progress: number },
  iterations: number
) => {
  return (measurement: any) => {
    if (measurement.Source === "client") {
      results.downloadMbps = Math.round(measurement.Data.MeanClientMbps)
    } else {
      iterations += 1
      results.progress = Math.ceil(
        (measurement.Data.TCPInfo.ElapsedTime /
          (TEST_DURATION_SECONDS * 1000)) *
          100
      )
      const jitterInMs = measurement.Data.TCPInfo.RTTVar / 1000
      results.jitter = Math.round(
        (results.jitter * (iterations - 1) + jitterInMs) / iterations
      )
    }

    // If enough time has passed but we aren't done with the test, emit what we have so far
    if (results.progress >= 100) {
      testComplete(true)
    }
  }
}

const networkAnalysisEvent = new events.EventEmitter()
const testComplete = (success: boolean) => {
  if (!emitted) {
    networkAnalysisEvent.emit("finished", success ? results : undefined)
    emitted = true
  }
}

// Callbacks to pass into download speed test
const callbacks = {
  downloadMeasurement: handleDownloadMeasurements(results, iterations),
  downloadComplete: () => {
    testComplete(true)
  },
  error: (err: { message: string }) => {
    console.error(err.message)
    testComplete(false)
  },
}

const networkAnalyze = () => {
  const urlPromise = ndt7.discoverServerURLs(config, callbacks)
  ndt7.downloadTest(config, callbacks, urlPromise)

  setTimeout(() => {
    testComplete(true)
  }, TEST_DURATION_SECONDS)
}

export { networkAnalysisEvent, networkAnalyze, TEST_DURATION_SECONDS }
