## AppArmor Profiles

This folder contains code to dump the default AppArmor profile which Docker uses, so that we may make necessary modifications to enable functionality such as FUSE.

### Profile Modifications

It is important to modify AppArmor and to not entirely remove it via `--security-opt apparmor:unconfined`, as we still want 99% of the security functionality provided! As we harden our mandelbox runtimes, we'll also want to strengthen this configuration, so it may become necessary to _expand_ the AppArmor profile.

This starts by grabbing the `docker-default` configuration and making these changes to the file.

### Context

The issue moby/moby#33060 tracks the progress of simplifying how AppArmor works and making it more transparent. In particular, the project wishes to make AppArmor function similarly to how Seccomp works in Docker -- easy to configure in `/etc/docker` and easy to load via JSON files. Importantly, the proposed changes would also make it very easy to see the contents of the currently-loaded `docker-default` AppArmor profile.

Unfortunately, work seems to have stalled there, so it's necessary for us to dump this information ourselves! The author of the above issue puts it very well in the issue description:

> Prior to Docker 1.13, it stored the AppArmor Profile in /etc/apparmor.d/docker-default (which was overwritten when Docker started, so users couldn't modify it. Docker devs added the `--security-opt` to let users specify a profile. After v1.13, Docker now generates docker-default in tmpfs, uses `apparmor_parser` to load it into kernel, _then deletes the file_. All of the AppArmor utils (aa-_ on Ubuntu) expect a file parameter, and /sys/kernel/security/apparmor/policy/profiles/_ only has cached binaries.

### Approach

Modify the file `profiles/apparmor/apparmor.go` from https://github.com/moby/moby to export relevant functions, and use those functions to dump the template-generated `docker-daemon` profile.

### Building

Make sure Go is installed on your system. Then, simply run `go build` to generate the binary.

### Usage

To dump the contents of `docker-daemon` to a file `file.profile`, simply run:

```bash
./apparmor-dump-profile > file.profile
```

After making any necessary modifications to the file, load it into AppArmor via:

```bash
apparmor_parser --replace --write-cache file.profile
```

To remove the profile, run:

```bash
apparmor_parser --remove file.profile
```

Once the profile has been loaded, launch a Docker container with the parameter `--security-opt apparmor:file.profile` to apply the new profile!
