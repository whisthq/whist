import ndt7 from "@m-lab/ndt7"

// Maximum amount of time we want the download test to take in seconds
const MICROSECONDS_IN_SECOND = 1000 * 1000
const TEST_DURATION_SECONDS = 5 * MICROSECONDS_IN_SECOND

// Config taken from @m-lab/ndt7 repo example
const config = { userAcceptedDataPolicy: true }
// The download test emits results over a period of time; create variables
// to track those results
let latencyJitterCount = 0
let results = {
  latency: 0,
  jitter: 0,
  download_mbps: 0,
  lost: 0,
  progress: 0,
}
// Handles download measurements
const handleDownloadMeasurements = (
  results: any,
  latencyJitterCount: number
) => {
  return (measurement: any) => {
    if (measurement.Source === "client") {
      results.download_mbps = measurement.Data.MeanClientMbps
    } else {
      console.log("Lost ", measurement.Data)
      latencyJitterCount += 1
      const phase_completed_percent =
        (measurement.Data.TCPInfo.ElapsedTime / TEST_DURATION_SECONDS) * 100
      results.progress = phase_completed_percent
      const measurement_latency_in_ms = measurement.Data.TCPInfo.RTT / 1000
      const measurement_jitter_in_ms = measurement.Data.TCPInfo.RTTVar / 1000
      results.latency =
        (results.latency * (latencyJitterCount - 1) +
          measurement_latency_in_ms) /
        latencyJitterCount
      results.jitter =
        (results.jitter * (latencyJitterCount - 1) + measurement_jitter_in_ms) /
        latencyJitterCount
      results.lost += measurement.Data.TCPInfo.Lost
    }
    if (results.progress >= 100) {
      console.log("Done!", results)
    }
  }
}
// Callbacks to pass into download speed test
const callbacks = {
  downloadMeasurement: handleDownloadMeasurements(results, latencyJitterCount),
  downloadComplete: () => {
    console.log("Done!", results)
  },
  error: (err: { message: string }) => {
    console.error(err.message)
  },
}

const urlPromise = ndt7.discoverServerURLs(config, callbacks)
ndt7.downloadTest(config, callbacks, urlPromise)
