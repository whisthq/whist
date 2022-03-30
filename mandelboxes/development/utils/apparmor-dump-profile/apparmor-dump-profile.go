package main

import (
        "os"
        "./apparmor"
)

func main() {
    p := apparmor.ProfileData{
        Name: "mandelbox-apparmor-profile",
        DaemonProfile: "unconfined",
    }
    p.GenerateDefault(os.Stdout)
    return
}
