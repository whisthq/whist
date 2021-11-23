import ndt7 from "@m-lab/ndt7"
import events from "events"

// Maximum amount of time we want the download test to take in seconds
const MILLISECONDS = 1000
const TEST_DURATION_SECONDS = 10 * MILLISECONDS

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

const networkAnalysisEvent = new events.EventEmitter()

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

    networkAnalysisEvent.emit("did-update", results)
  }
}

// Callbacks to pass into download speed test
const callbacks = {
  downloadMeasurement: handleDownloadMeasurements(results, iterations),
  downloadComplete: () => {
    networkAnalysisEvent.emit("did-update", { ...results, progress: 100 })
  },
  error: (err: { message: string }) => {
    console.error(err.message)
    networkAnalysisEvent.emit("did-update", { ...results, progress: 100 })
  },
}

const networkAnalyze = () => {
  const urlPromise = ndt7.discoverServerURLs(config, callbacks)
  ndt7.downloadTest(config, callbacks, urlPromise)

  setTimeout(() => {
    networkAnalysisEvent.emit("did-update", { ...results, progress: 100 })
  }, TEST_DURATION_SECONDS)
}

export { networkAnalysisEvent, networkAnalyze, TEST_DURATION_SECONDS }
