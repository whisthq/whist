# Protocol Test Frameworks

This folder contains the test frameworks we use for testing the `/protocol` codebase. We currently have two test frameworks: unit tests and an end-to-end streaming test.

## Unit Tests

The point of unit tests is to test specific parts of the code when taken in isolation and to check that they conform to the specifications. Unit tests are also a good place to test for corner cases and unexpected inputs.

We use the [Google Test](https://google.github.io/googletest/) suite to compile and run our tests. You can add more tests by editing the `protocol_test.cpp` file in this folder. A typical unit test in Google Test format is as follows:

```c++
TEST(TestGroup, TestName) {
    <test body here, in C++ format>
}
```

Google Test also support fixtures. You can read more about it [here](https://google.github.io/googletest/primer.html#same-data-multiple-tests). In our test suite, we use fixtures to be able to check the output printed to stdout.

For reference, see the first unit test from `protocol_test.cpp`, copied below. In this example, we are using the `ProtocolTest` fixture, which includes functions to check the stdout. To use the fixture, we use the `TEST_F` keyword instead of `TEST`.

```c++
TEST_F(ProtocolTest, ClientParseHelp) {
    const char* argv[] = {"./wclient", "--help"};
    int argc = ARRAY_LENGTH(argv);
    EXPECT_EXIT(client_parse_args(argc, argv), testing::ExitedWithCode(0), "");
    check_stdout_line(::testing::StartsWith("Usage:"));
}
```

## The End-to-End Streaming test

The End-to-End streaming test is an integration test which verifies that all components of the Whist stack (with the exception of the client-app layer) properly interact together to produce the desired overall behavior. In addition to checking that no fatal errors are present at the host-setup, host-service, mandelbox and protocol layers, the E2E test can also be used as a benchmarking suite.

Running the E2E script in this folder will allow to run the Whist server and client in a controlled and stable environment, thus it will produce (hopefully) reproducible results. The client and server will each live inside a mandelbox, and the two mandelbox will run on either the same or two separate AWS instances, depending on the user's settings.

The code in this folder will allow you to run the test on one/two AWS instances and download all the monitoring and benchmarking logs. The scripts can run either on a local machine, or within a CI workflow. The code for the `Protocol: End-to-End Streaming Testing` workflow is not in this folder, and can be found [here](../../.github/workflows/protocol-e2e-streaming-testing.yml). For more details on the workflow, check out the corresponding [section below](#the-protocol-end-to-end-streaming-testing-ci-workflow).

### Usage

You can run the E2E streaming test by launching the script `streaming_e2e_tester.py` in this folder. Running the script on a local machine will create or start one or two EC2 instances (depending on the parameters you pass in), run the test there, and save all the logs on the local machine. The logs will be stored at the path `./perf_logs/<test_start_time>/`, where `<test_start_time>` is a timestamp of the form `YYYY-MM-DD@hh-mm-ss`

The script's arguments are summarized below. Three of them (`--ssh-key-name SSH_KEY_NAME`, `--ssh-key-path SSH_KEY_PATH` `--github-token GITHUB_TOKEN `) are required, the remaining ones can be omitted if you want to use the default values. Scroll below to see the arguments that can be used for sample use cases.

```
usage: streaming_e2e_tester.py [-h] --ssh-key-name SSH_KEY_NAME --ssh-key-path SSH_KEY_PATH --github-token GITHUB_TOKEN
                               [--region-name {us-east-1,us-east-2,us-west-1,us-west-2,af-south-1,ap-east-1,ap-south-1,ap-northeast-3,ap-northeast-2,ap-southeast-1,ap-southeast-2,ap-northeast-1,ca-central-1,eu-central-1,eu-west-1,eu-west-2,eu-south-1,eu-west-3,eu-north-1,sa-east-1}]
                               [--use-existing-server-instance USE_EXISTING_SERVER_INSTANCE] [--use-existing-client-instance USE_EXISTING_CLIENT_INSTANCE] [--use-two-instances {false,true}]
                               [--leave-instances-on {false,true}] [--skip-host-setup {false,true}] [--skip-git-clone {false,true}] [--simulate-scrolling {false,true}] [--testing-url TESTING_URL]
                               [--testing-time TESTING_TIME] [--cmake-build-type {dev,prod,metrics}] [--aws-credentials-filepath AWS_CREDENTIALS_FILEPATH] [--network-conditions NETWORK_CONDITIONS]
                               [--aws-timeout-seconds AWS_TIMEOUT_SECONDS] [--username USERNAME] [--ssh-connection-retries SSH_CONNECTION_RETRIES]

This script will spin one or two g4dn.xlarge EC2 instances (depending on the parameters you pass in), start two Docker containers (one for the Whist client, one for the Whist server), and run a protocol
streaming performance test between the two of them. In case one EC2 instance is used, the two Docker containers are started on the same instance.

required arguments:
  --ssh-key-name SSH_KEY_NAME
                        The name of the AWS RSA SSH key you wish to use to access the E2 instance(s). If you are running the script locally, the key name is likely of the form <yourname-key>. Make sure to
                        pass the key-name for the region of interest. Required.
  --ssh-key-path SSH_KEY_PATH
                        The path to the .pem certificate on this machine to use in connection to the RSA SSH key passed to the --ssh-key-name arg. Required.
  --github-token GITHUB_TOKEN
                        The GitHub Personal Access Token with permission to fetch the whisthq/whist repository. Required.

optional arguments:
  -h, --help            show this help message and exit
  --region-name {us-east-1,us-east-2,us-west-1,us-west-2,af-south-1,ap-east-1,ap-south-1,ap-northeast-3,ap-northeast-2,ap-southeast-1,ap-southeast-2,ap-northeast-1,ca-central-1,eu-central-1,eu-west-1,eu-west-2,eu-south-1,eu-west-3,eu-north-1,sa-east-1}
                        The AWS region to use for testing. If you are looking to re-use an instance for the client and/or server, the instance(s) must live on the region passed to this parameter.
  --use-existing-server-instance USE_EXISTING_SERVER_INSTANCE
                        The instance ID of an existing instance to use for the Whist server during the E2E test. You can only pass a value to this parameter if you passed `true` to --use-two-instances.
                        Otherwise, the server will be installed and run on the same instance as the client. The instance will be stopped upon completion. If left empty (and --use-two-instances=true), a clean
                        new instance will be generated instead, and it will be terminated (instead of being stopped) upon completion of the test.
  --use-existing-client-instance USE_EXISTING_CLIENT_INSTANCE
                        The instance ID of an existing instance to use for the Whist dev client during the E2E test. If the flag --use-two-instances=false is used (or if the flag --use-two-instances is not
                        used), the Whist server will also run on this instance. The instance will be stopped upon completion. If left empty, a clean new instance will be generated instead, and it will be
                        terminated (instead of being stopped) upon completion of the test.
  --use-two-instances {false,true}
                        Whether to run the client on a separate AWS instance, instead of the same as the server.
  --leave-instances-on {false,true}
                        This option allows you to override the default behavior and leave the instances running upon completion of the test, instead of stopping (if reusing existing ones) or terminating (if
                        creating new ones) them.
  --skip-host-setup {false,true}
                        This option allows you to skip the host-setup on the instances to be used for the test. This will save you a good amount of time.
  --skip-git-clone {false,true}
                        This option allows you to skip cloning the Whist repository on the instance(s) to be used for the test. The test will also not checkout the current branch or pull from Github, using
                        the code from the whist folder on the existing instance(s) as is. This option is useful if you need to run several tests in succession using code from the same commit.
  --simulate-scrolling {false,true}
                        Whether to simulate mouse scrolling on the client side
  --testing-url TESTING_URL
                        The URL to visit during the test. The default is a 4K video stored on S3.
  --testing-time TESTING_TIME
                        The time (in seconds) to wait at the URL from the `--testing-url` flag before shutting down the client/server and grabbing the logs and metrics. The default value is the duration of
                        the 4K video from S3 mentioned above.
  --cmake-build-type {dev,prod,metrics}
                        The Cmake build type to use when building the protocol.
  --aws-credentials-filepath AWS_CREDENTIALS_FILEPATH
                        The path (on the machine running this script) to the file containing the AWS credentials to use to access the Whist AWS console. The file should contain the access key id and the
                        secret access key. It is created/updated when running `aws configure`
  --network-conditions NETWORK_CONDITIONS
                        The network condition for the experiment. The input is in the form of three comma-separated floats indicating the max bandwidth, delay (in ms), and percentage of packet drops (in the
                        range [0.0,1.0]). 'normal' will allow the network to run with no degradation. For example, pass --network-conditions 1Mbit,100,0.1 to simulate a bandwidth of 1Mbit/s, 100ms delay and
                        10 percent probability of packet drop
  --aws-timeout-seconds AWS_TIMEOUT_SECONDS
                        The timeout after which we give up on commands that have not finished on a remote AWS E2 instance. This value should not be set to less than 20mins (1200s)
  --username USERNAME   The username to use to access the AWS EC2 instance(s)
  --ssh-connection-retries SSH_CONNECTION_RETRIES
                        The number of times to retry if a SSH connection is refused or if the connection attempt times out
```

#### Sample usage 1 (default): create a fresh new instance on us-east-1 and terminate it upon completion

```
python3 streaming_e2e_tester.py --ssh-key-name <yourname-key> --ssh-key-path </path/to/yourname-key.pem> --github-token <your-github-token-here>
```

#### Sample usage 2: create 2 new instances on ap-south-2 (Mumbai), simulate scrolling during the test, and leave instances on upon completion

```
python3 streaming_e2e_tester.py --ssh-key-name <yourname-key> --ssh-key-path </path/to/yourname-key.pem> --github-token <your-github-token-here --region-name ap-south-2 --simulate-scrolling=true --leave-instances-on=true>
```

#### Sample usage 3: test on your dev instance on us-west-1, skipping the host-setup. Leave the instance on upon completion. Simulate a degraded network with a bandwidth of 1Mbit/s, 100ms delay and 10% probability of packet drop

```
python3 streaming_e2e_tester.py --ssh-key-name <yourname-key> --ssh-key-path </path/to/yourname-key.pem> --github-token <your-github-token-here --region-name us-west-1 --use-existing-client-instance <your-dev-instance-id> --leave-instances-on=true --skip-host-setup=true --network-conditions 1Mbit,100,0.1
```

---

### The `Protocol: End-to-End Streaming Testing` CI workflow
The `Protocol: End-to-End Streaming Testing` workflow defines a Github CI Action that automatically runs the End-To-End streaming test within a Github runner, retrieves and saves the logs to a S3 bucket, and post the results to Github Gist, as a comment on a PR body, and/or on Slack, depending on the settings.

#### Triggers
The `Protocol: End-to-End Streaming Testing` workflow is currently set up so that it is triggered by 3 events:

1. `schedule`: on weekdays at 00:01am UTC, we run the test on the default branch (`dev`), using the latest commit.
2. `pull_request`: when a commit is made to a branch that is associated to an open pull request, and the commit modifies prototocol-related files (or the E2E test files) we run the test as part of the PR checks
3. `workflow_dispatch`: the test can also be run manually by triggering the workflow from [this page.](https://github.com/whisthq/whist/actions/workflows/protocol-e2e-streaming-testing.yml)

#### Network conditions
The `Protocol: End-to-End Streaming Testing` workflow runs the E2E test a total of 4 times, one for each of the following network conditions on the client EC2 instance:

1. _Normal condition_: no artificial network condition degradation is applied
2. _Reduced bandwidth_: the client bandwidth is artificially limited to a max transmit/receive rate of 1Mbit/s
3. _Reduced bandwidth and packet drops_: the client bandwidth is artificially limited to a max transmit/receive rate of 1Mbit/s, and incoming/outgoing packets are dropped with a probability of 10%
4. _Reduced bandwidth, packet drops, and delay_: the client bandwidth is artificially limited to a max transmit/receive rate of 1Mbit/s, and incoming/outgoing packets are dropped with a probability of 10%, and an additional delay of 100ms is applied to all traffic.

#### Instances used
Running the test on fresh instances is relatively time-consuming, mainly due to the time that it takes to build the `mandelbox/base` Docker container, upon which both the server (`browsers/chrome`) and the client (`development/client`) mandelbox are based. More specifically, each E2E run on a fresh instance currently takes about 40mins, so testing under 4 different network conditions would take a total of 160mins.
We can significantly reduce the test time by reusing instances, which speeds up the build phase thanks to caching. To do so, we created the following two instances on `us-east-1`:

1. `protocol-integration-tester-client` with instance ID: `i-061d322780ae41d19`
2. `protocol-integration-tester-server` with instance ID: `i-0ceaa243ec8cfc667`

These instances are started every time a `Protocol: End-to-End Streaming Testing` workflow is run, and stopped once the workflow completes. 

If you need to access the two machines above, you can do so by using the key name `GITHUB_ACTIONS_E2E_PERFORMANCE_TEST_SSH_KEYPAIR` and the corresponding certificate `GITHUB_ACTIONS_E2E_PERFORMANCE_TEST_SSH_KEYPAIR.pem`, which is stored on S3 in the `fractal-dev-secrets/E2E_test_secrets/` folder.

#### Turnstyle
Because multiple instances of the `Protocol: End-to-End Streaming Testing` can run at once, and we can only run one test at a time on the two instances above, we need a mechanism for the workflow instances to coordinate, so that no two of them run concurrently. We can do this through the Turnstyle Github Action, using [this version](https://github.com/rpadaki/turnstyle) from @rpadaki. 

#### Posting logs to S3
Upon completion of the test (no matter if the steps succeeded, failed, were cancelled, or were skipped), we upload all protocol logs to S3. The logs are available at the path `s3://whist-e2e-protocol-test-logs/<branch_name>/datetime`. 

After a run of the workflow, you can quickly download the logs from S3 by copying the commands that are printed at the end of the `Upload logs to S3 for debugging purposes` step output on CI and running them in your terminal.

#### Posting results
Upon completion of the test, the `Protocol: End-to-End Streaming Testing` posts the results on a secret Github Gist, which is accessible by the link that is printed at the end of the `Parse & Display Test Results` step output on CI. For nightly runs on `dev`, we also post the result on Slack, in the `alerts-dev` channel. When the workflow is triggered by a commit to a branch with an open PR, we also post the results as a comment to that PR's body.

