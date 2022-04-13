# Protocol Test Frameworks

This folder contains the test frameworks we use for testing the `/protocol` codebase. We currently have two test frameworks: unit tests and an end-to-end streaming test.

## Unit Tests

The point of unit tests is to test specific parts of the code when taken in isolation and to check that they conform to the specifications. Unit tests are also a good place to test for corner cases and unexpected inputs/outputs.

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

## The End-to-End Streaming Test

The End-to-End streaming test is an integration test which is used to test the performance of the Whist streaming protocol in a controlled environment. It is essential to evaluating quantitatively whether the work we do on our streaming algorithm is impactful and works across a wide range of networks.

Running the E2E script in this folder will allow to run the Whist server and client in a controlled and stable environment, thus it will produce (hopefully) reproducible results. The client and server each live inside a Docker container, and the two containers run on either the same or two separate AWS EC2 instances, depending on the chosen test settings, performing a streaming test for a few minutes before reporting logs. The scripts can be run either from a local machine, or within a CI workflow.

### Usage

You can run the E2E streaming test by launching the script `streaming_e2e_tester.py` in this folder. Running the script on a local machine will create or start one or two EC2 instances (depending on the parameters you pass in), run the test there, and save all the logs on the local machine. The logs will be stored at the path `./perf_logs/<test_start_time>/`, where `<test_start_time>` is a timestamp of the form `YYYY-MM-DD@hh-mm-ss`

The script's arguments are summarized below. Three of them are required (`--ssh-key-name SSH_KEY_NAME`, `--ssh-key-path SSH_KEY_PATH` `--github-token GITHUB_TOKEN `). The remaining ones can be omitted if you want to use the default values.

```bash
usage: streaming_e2e_tester.py [-h] --ssh-key-name SSH_KEY_NAME --ssh-key-path SSH_KEY_PATH --github-token GITHUB_TOKEN
                               [--region-name {us-east-1,us-east-2,us-west-1,us-west-2,af-south-1,ap-east-1,ap-south-1,ap-northeast-3,ap-northeast-2,ap-southeast-1,ap-southeast-2,ap-southeast-3,ap-northeast-1,ca-central-1,eu-central-1,eu-west-1,eu-west-2,eu-south-1,eu-west-3,eu-north-1,sa-east-1}]
                               [--use-existing-server-instance USE_EXISTING_SERVER_INSTANCE] [--use-existing-client-instance USE_EXISTING_CLIENT_INSTANCE] [--use-two-instances {false,true}]
                               [--leave-instances-on {false,true}] [--skip-host-setup {false,true}] [--skip-git-clone {false,true}] [--simulate-scrolling {false,true}] [--testing-url TESTING_URL]
                               [--testing-time TESTING_TIME] [--cmake-build-type {dev,prod,metrics}] [--aws-credentials-filepath AWS_CREDENTIALS_FILEPATH] [--network-conditions NETWORK_CONDITIONS]
                               [--aws-timeout-seconds AWS_TIMEOUT_SECONDS] [--username USERNAME] [--ssh-connection-retries SSH_CONNECTION_RETRIES]

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

  --region-name {us-east-1,us-east-2,us-west-1,us-west-2,af-south-1,ap-east-1,ap-south-1,ap-northeast-3,
                ap-northeast-2, ap-southeast-1,ap-southeast-2,ap-southeast-3,ap-northeast-1,ca-central-1,eu-central-1,eu-west-1,eu-west-2,eu-south-1,eu-west-3,eu-north-1,sa-east-1}

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

  --simulate-scrolling NUM_SCROLLING_ROUNDS
                        Number of rounds of scrolling that should be simulated on the client side. One round of scrolling = Slow scroll down + Slow scroll up + Fast scroll down + Fast scroll up

  --testing-url TESTING_URL
                        The URL to visit during the test. The default is a 4K video stored on our AWS S3.

  --testing-time TESTING_TIME
                        The time (in seconds) to wait at the URL from the `--testing-url` flag before shutting down the client/server and grabbing the logs and metrics. The default value is the duration of
                        the 4K video from our AWS S3.

  --cmake-build-type {dev,prod,metrics}
                        The Cmake build type to use when building the protocol.

  --aws-credentials-filepath AWS_CREDENTIALS_FILEPATH
                        The path (on the machine running this script) to the file containing the AWS credentials to use to access the Whist AWS console. The file should contain the access key id and the
                        secret access key. It is created/updated when running `aws configure`

  --network-conditions NETWORK_CONDITIONS
                        The network condition for the experiment. The input is in the form of three comma-separated floats indicating the max bandwidth, delay (in ms), and percentage of packet drops (in the
                        range [0.0,1.0]). 'normal' will allow the network to run with no degradation. For example, pass --network-conditions 1Mbit,100,0.1 to simulate a bandwidth of 1Mbit/s, 100ms delay and
                        10 percent probability of packet drop.

  --aws-timeout-seconds AWS_TIMEOUT_SECONDS
                        The timeout after which we give up on commands that have not finished on a remote AWS E2 instance. This value should not be set to less than 20mins (1200s).

  --username USERNAME
                        The username to use to access the AWS EC2 instance(s)

  --ssh-connection-retries SSH_CONNECTION_RETRIES
                        The number of times to retry if a SSH connection is refused or if the connection attempt times out
```

#### Sample Usage

Here are a few examples of how to use the test framework.

**Run on 1 new instance on `us-east-1`, terminated upon completion**

```bash
python3 streaming_e2e_tester.py --ssh-key-name <yourname-key> --ssh-key-path </path/to/yourname-key.pem> --github-token <your-github-token-here>
```

**Run on 2 new instances on `ap-south-2`, simulate scrolling during test, and leave instances on upon completion**

```bash
python3 streaming_e2e_tester.py --ssh-key-name <yourname-key> --ssh-key-path </path/to/yourname-key.pem> --github-token <your-github-token-here --region-name ap-south-2 --simulate-scrolling=10 --testing-url="https://docs.nvidia.com/video-technologies/video-codec-sdk/nvenc-video-encoder-api-prog-guide/" --leave-instances-on=true>
```

**Run on your development instance on `us-west-` with degraded network (1Mbit/s, 100ms delay, 10% packet drop), and leave instance on upon completion**

```bash
python3 streaming_e2e_tester.py --ssh-key-name <yourname-key> --ssh-key-path </path/to/yourname-key.pem> --github-token <your-github-token-here --region-name us-west-1 --use-existing-client-instance <your-dev-instance-id> --leave-instances-on=true --skip-host-setup=true --network-conditions 1Mbit,100,0.1
```

#### Creating New Experiments & Network Conditions

The E2E test framework can run arbitrarily many experiments for any given set of network conditions. To do this, you can pass the `--network-conditions` flag with a comma-separated list of Bandwidth (in Mbps),Delay (in ms),PacketDrop (in percetange, between 0 and 1).

You can see the list of all the experiments we run, and their network conditions, in the `protocol-e2e-streaming-testing.yml`, and you can add your new experiments there if you believe they are representative of a network condition we are not currently testing in CI.

### Development

The code for the E2E test framework is shared between this folder (general components) and the `.github/workflows/helpers/protocol` folders (CI-specific components). When contributing to this project, please keep your work scoped to the right folders and keep the code simple and abstracted. Note that we sometimes modify protocol logs and modify/add network experiments, so please try to keep your work as abstracted as possible to keep the documentation and code in sync.

As of writing, our CI E2E tests can only run sequentially. To achieve this, we use (Turnstyle)[(https://github.com/rpadaki/turnstyle)]. You can of course run tests on your own development instance irrespective of our CI instances.

#### CI Test Instances

To speed up the testing process in CI, we reuse two AWS EC2 instances dedicated to the E2E tests:

1. `protocol-integration-tester-client` with instance ID: `i-061d322780ae41d19`
2. `protocol-integration-tester-server` with instance ID: `i-0ceaa243ec8cfc667`

If you need to access the two machines above (i.e. to debug something), you can do so by using the key name `GITHUB_ACTIONS_E2E_PERFORMANCE_TEST_SSH_KEYPAIR` and the corresponding certificate `GITHUB_ACTIONS_E2E_PERFORMANCE_TEST_SSH_KEYPAIR.pem`, which is stored on AWS S3 in the `whist-dev-secrets/E2E_test_secrets/` folder.

### Deployment

The test gets run automatically via the GitHub Actions workflow `protocol-e2e-streaming-testing.yml` nightly at midnight UTC for the `dev` branch, and for all PRs to `dev`, for the originating branch. The results are published as a comment to the originating PR, or to Slack in the case of the nightly runs.
