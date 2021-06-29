package main

import (
        "os"
        "./apparmor"
)

func main() {
    p := apparmor.ProfileData{
        Name: "docker-default",
        DaemonProfile: "unconfined",
    }
    p.GenerateDefault(os.Stdout)
    return
}
