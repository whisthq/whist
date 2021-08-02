package metadata // import "github.com/fractal/fractal/host-service/metadata"

// Fractal webserver URLs and relevant webserver endpoints
const localdevFractalWebserver = "https://127.0.0.1:7730"
const devFractalWebserver = "https://dev-server.fractal.co"
const stagingFractalWebserver = "https://staging-server.fractal.co"
const prodFractalWebserver = "https://prod-server.fractal.co"

// GetFractalWebserver returns the appropriate webserver URL based on whether
// we're running in production, staging or development. Since GetAppEnvironment
// caches its result, we don't need to cache this.
func GetFractalWebserver() string {
	switch GetAppEnvironment() {
	case EnvStaging:
		return stagingFractalWebserver
	case EnvProd:
		return prodFractalWebserver
	case EnvDev:
		return devFractalWebserver
	case EnvLocalDev, EnvLocalDevWithDB:
		return localdevFractalWebserver
	default:
		return localdevFractalWebserver
	}
}
