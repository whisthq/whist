package metadata // import "github.com/fractal/fractal/host-service/metadata"

// Variable for hash of last Git commit --- filled in by linker
var gitCommit string

// GetGitCommit returns the git commit hash of this build.
func GetGitCommit() string {
	return gitCommit
}
