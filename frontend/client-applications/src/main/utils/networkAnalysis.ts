import ndt7 from "@m-lab/ndt7"
import events from "events"

// Maximum amount of time we want the download test to take in seconds
const MILLISECONDS = 1000
const TEST_DURATION_MILLISECONDS = 8 * MILLISECONDS
// Sometimes events can emit out of order so we track the max progress to make sure we're not emitting
// an earlier event
let MAX_PROGRESS = 0

// Config taken from @m-lab/ndt7 repo example
const config = { userAcceptedDataPolicy: true }

// The download test emits results over a period of time; create variables
// to track those results
let results = {
  jitter: 0,
  downloadMbps: 0,
  progress: 0,
}

const networkAnalysisEvent = new events.EventEmitter()

const emitNetworkAnalysis = (_results: {
  jitter: number
  downloadMbps: number
  progress: number
}) => {
  if (_results.progress >= MAX_PROGRESS && MAX_PROGRESS < 100) {
    networkAnalysisEvent.emit("did-update", _results)
    MAX_PROGRESS = _results.progress
  }
}

// Handles download measurements
const handleDownloadMeasurements = () => {
  return (measurement: any) => {
    if (measurement.Source === "client") {
      results.downloadMbps = Math.round(measurement.Data.MeanClientMbps)
    } else {
      results.progress = Math.ceil(
        (measurement.Data.TCPInfo.ElapsedTime /
          (TEST_DURATION_MILLISECONDS * 1000)) *
          100
      )
      const jitterInMs = Math.ceil(measurement.Data.TCPInfo.RTTVar / 1000)
      results.jitter = Math.max(results.jitter, jitterInMs)
    }

    emitNetworkAnalysis(results)
  }
}

// Callbacks to pass into download speed test
const callbacks = {
  downloadMeasurement: handleDownloadMeasurements(),
  error: (err: { message: string }) => {
    console.error(err.message)
    emitNetworkAnalysis({ ...results, progress: 100 })
  },
}

const networkAnalyze = () => {
  MAX_PROGRESS = 0
  results = {
    jitter: 0,
    downloadMbps: 0,
    progress: 0,
  }

  const urlPromise = ndt7.discoverServerURLs(config, callbacks)
  ndt7.downloadTest(config, callbacks, urlPromise)

  setTimeout(() => {
    emitNetworkAnalysis({ ...results, progress: 100 })
  }, TEST_DURATION_MILLISECONDS)
}

export { networkAnalysisEvent, networkAnalyze, TEST_DURATION_MILLISECONDS }
