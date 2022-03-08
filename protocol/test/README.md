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
TEST_F(ProtocolTest, ClientParseArgsEmpty) {
    int argc = 1;

    char argv0[] = "./client/build64/WhistClient";
    char* argv[] = {argv0, NULL};

    int ret_val = client_parse_args(argc, argv);
    EXPECT_EQ(ret_val, -1);

    check_stdout_line(::testing::StartsWith("Usage:"));
    check_stdout_line(::testing::HasSubstr("--help"));
}
```

## The End-to-End Streaming test

The End-to-End streaming test is an integration test which verifies that all components of the Whist stack (with the exception of the client-app layer) properly interact together to produce the desired overall behavior. In addition to checking that no fatal errors are present at the host-setup, host-service, mandelbox and protocol layers, the E2E test can also be used as a benchmarking suite.

Running the E2E script in this folder will allow to run the Whist server and client in a controlled and stable environment, thus it will produce (hopefully) reproducible results. The client and server will each live inside a mandelbox, and the two mandelbox will run on either the same or two separate AWS instances, depending on the user's settings.

The code in this folder will allow you to run the test on one/two AWS instances and download all the monitoring and benchmarking logs. The scripts can run either on a local machine, or within a CI workflow. The code for the `Protocol: End-to-End Streaming Testing` workflow is not in this folder, and can be found [here](../../.github/workflows/protocol-e2e-streaming-testing.yml). For more details on the workflow, check out the corresponding section below.

### Usage

You can run the E2E streaming test by launching the script `streaming_e2e_tester.py` in this folder. Running the script on a local machine will create or start one/two E2 instances, run the test there, and save all the logs on the local machine. The logs will be stored at the path `./perf_logs/<test_start_time>/`, where `<test_start_time>` is a timestamp of the form `YYYY-MM-DD@hh-mm-ss`

The script's arguments are summarized below. Three of them (`--ssh-key-name SSH_KEY_NAME`, `--ssh-key-path SSH_KEY_PATH` `--github-token GITHUB_TOKEN `) are required, the remaining ones can be omitted if you want to use the default values. Scroll below to see the arguments that can be used for sample use cases.

```
usage: streaming_e2e_tester.py [-h] --ssh-key-name SSH_KEY_NAME --ssh-key-path SSH_KEY_PATH --github-token GITHUB_TOKEN
                               [--region-name {us-east-1,us-east-2,us-west-1,us-west-2,af-south-1,ap-east-1,ap-south-1,ap-northeast-3,ap-northeast-2,ap-southeast-1,ap-southeast-2,ap-northeast-1,ca-central-1,eu-central-1,eu-west-1,eu-west-2,eu-south-1,eu-west-3,eu-north-1,sa-east-1}]
                               [--use-two-instances {false,true}] [--use-existing-client-instance USE_EXISTING_CLIENT_INSTANCE]
                               [--use-existing-server-instance USE_EXISTING_SERVER_INSTANCE] [--leave-instances-on {false,true}] [--skip-host-setup {false,true}]
                               [--skip-git-clone {false,true}] [--simulate-scrolling {false,true}] [--testing-url TESTING_URL] [--testing-time TESTING_TIME]
                               [--cmake-build-type {dev,prod,metrics}] [--aws-credentials-filepath AWS_CREDENTIALS_FILEPATH] [--network-conditions NETWORK_CONDITIONS]

This script will spin a g4dn.xlarge EC2 instance with two Docker containers and run a protocol streaming performance test between the two of them. Optionally, the two
Docker containers can be run on two separate g4dn.xlarge EC2 instances.

required arguments:
  --ssh-key-name SSH_KEY_NAME
                        The name of the AWS RSA SSH key you wish to use to access the E2 instance(s). If you are running the script locally, the key name is likely of
                        the form <yourname-key>. Make sure to pass the key-name for the region of interest. Required.
  --ssh-key-path SSH_KEY_PATH
                        The path to the .pem certificate on this machine to use in connection to the RSA SSH key passed to the --ssh-key-name arg. Required.
  --github-token GITHUB_TOKEN
                        The GitHub Personal Access Token with permission to fetch the whisthq/whist repository. Required.

optional arguments:
  -h, --help            show this help message and exit
  --region-name {us-east-1,us-east-2,us-west-1,us-west-2,af-south-1,ap-east-1,ap-south-1,ap-northeast-3,ap-northeast-2,ap-southeast-1,ap-southeast-2,ap-northeast-1,ca-central-1,eu-central-1,eu-west-1,eu-west-2,eu-south-1,eu-west-3,eu-north-1,sa-east-1}
                        The AWS region to use for testing. If you are looking to re-use an instance for the client and/or server, the instance(s) must live on the
                        region passed to this parameter.
  --use-two-instances {false,true}
                        Whether to run the client on a separate AWS instance, instead of the same as the server.
  --use-existing-client-instance USE_EXISTING_CLIENT_INSTANCE
                        The instance ID of an existing instance to use for the Whist dev client during the E2E test. If the flag --use-two-instances=false is used (or
                        if the flag --use-two-instances is not used), the Whist server will also run on this instance. The instance will be stopped upon completion.
                        If left empty, a clean new instance will be generated instead, and it will be terminated (instead of being stopped) upon completion of the
                        test.
  --use-existing-server-instance USE_EXISTING_SERVER_INSTANCE
                        The instance ID of an existing instance to use for the Whist server during the E2E test. You can only pass a value to this parameter if you
                        passed `true` to --use-two-instances. Otherwise, the server will be installed and run on the same instance as the client. The instance will be
                        stopped upon completion. If left empty (and --use-two-instances=true), a clean new instance will be generated instead, and it will be
                        terminated (instead of being stopped) upon completion of the test.
  --leave-instances-on {false,true}
                        This option allows you to override the default behavior and leave the instances running upon completion of the test, instead of stopping (if
                        reusing existing ones) or terminating (if creating new ones) them.
  --skip-host-setup {false,true}
                        This option allows you to skip the host-setup on the instances to be used for the test. This will save you a good amount of time.
  --skip-git-clone {false,true}
                        This option allows you to skip cloning the Whist repository on the instance(s) to be used for the test. The test will also not checkout the
                        current branch or pull from Github, using the code from the whist folder on the existing instance(s) as is. This option is useful if you need
                        to run several tests in succession using code from the same commit.
  --simulate-scrolling {false,true}
                        Whether to simulate mouse scrolling on the client side
  --testing-url TESTING_URL
                        The URL to visit during the test. The default is a 4K video stored on S3.
  --testing-time TESTING_TIME
                        The time (in seconds) to wait at the URL from the `--testing-url` flag before shutting down the client/server and grabbing the logs and
                        metrics. The default value is the duration of the 4K video from S3 mentioned above.
  --cmake-build-type {dev,prod,metrics}
                        The Cmake build type to use when building the protocol.
  --aws-credentials-filepath AWS_CREDENTIALS_FILEPATH
                        The path (on the machine running this script) to the file containing the AWS credentials to use to access the Whist AWS console. The file
                        should contain the access key id and the secret access key. It is created/updated when running `aws configure`
  --network-conditions NETWORK_CONDITIONS
                        The network condition for the experiment. The input is in the form of three comma-separated floats indicating the max bandwidth, delay (in
                        ms), and percentage of packet drops (in the range [0.0,1.0]). 'normal' will allow the network to run with no degradation. For example, pass
                        --network-conditions 1Mbit,100,0.1 to simulate a bandwidth of 1Mbit/s, 100ms delay and 10 percent probability of packet drop
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

### The `Protocol: End-to-End Streaming Testing` workflow

TODO:

1. Discuss existing instances and how to access them in case of need
2. Discuss network degradation conditions
3. Discuss how to find logs on S3
4. Discuss parsing of results
