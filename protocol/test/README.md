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

The End-to-End streaming test is an integration test with a twofold purpose. First, it ensures that all layers of the Whist stack are able to work together as needed. Second, it tests the performance of the Whist streaming protocol in a controlled environment. It is essential to evaluating quantitatively whether the work we do on our streaming algorithm is impactful and works across a wide range of networks.

Running the E2E script in this folder will allow to run the Whist server and client in a controlled and stable environment, thus it will produce (hopefully) reproducible results. The client and server each live inside a Docker container, and the two containers run on either the same or two separate AWS EC2 instances, depending on the chosen test settings, performing a streaming test for a few minutes before reporting logs. The scripts can be run either from a local machine, or within a CI workflow.

### Usage

You can run the E2E streaming test by setting the proper configurations in the `e2e_helpers/configs/default.yaml` file and then launching the script `streaming_e2e_tester.py` in this folder.

#### 1. Setting the configurations

To reduce the number of arguments that need to be passed to the `streaming_e2e_tester.py` script at every run, we load all the configurations from a yaml file (`default.yaml` when running the script manually, `e2e.yaml` and `backend_integration.yaml` when the script is run in CI). The first step to run the E2E manually is to edit the `default.yaml` file and insert the desired values for at least the required parameters (`ssh_key_name`, `ssh_key_path`, `github_token`). All parameters are described thoroughly in the comments sections. Make sure to review the values passed to the optional parameters to ensure that they are correct for your setup. For instance, if you want to use your dev instance for the E2E server/client, you will have to fill in your instance-id in the `existing_server_instance_id` field, and set the `region_name` to the region where the instance is located. If you want to run the E2E server and client on two separate instances, you should set `use_two_instances: "true"` and either leave the `existing_server_instance_id` and `existing_client_instance_id` blank if you want the E2E to create two fresh instances, or pass instance-ids if you want to run the test using existing instances. Leaving the region_name blank will let the E2E pick which region to use depending on availability. However, if your ssh key only works in a single region, or if you are reusing one/two existing instances, you cannot leave the region_name blank. Instead, you will have to specify the region where your key / the existing instance(s) are.

N.B.: The `default.yaml` file has been configured so that Git does not, by default, track any local changes. This prevents you from accidentally uploading to GitHub your configurations (including your credentials) when you commit all modified files. If you do want to commit changes to the `default.yaml` file, you will have to run `git update-index --no-assume-unchanged e2e_helpers/configs/default.yaml` to ask Git to track the changes, commit, and then run `git update-index --assume-unchanged e2e_helpers/configs/default.yaml` to once again turn off the tracking of new changes to the file.

#### 2. Running the script

To run the script, simply call `python3 streaming_e2e_tester.py` from the `protocol/test` folder. No other argument is required, unless you want to override the configs from the `default.yaml` config file. All optional parameters (to override the configs) are listed in `e2e_helpers/configs/parse_configs.py`. Using command-line arguments is especially helpful if you want to rerun the same experiment under different conditions. For instance, if you want to visit the same website with the same test duration, but using different network conditions, you will pass a value to the `--network-conditions` command-line parameter, and the script will get all the other configs from the `default.yaml` file.

Running the script on a local machine will create or start one or two EC2 instances (depending on the parameters you pass in), run the test there, and save all the logs on the local machine. The logs will be stored at the path `./e2e_logs/<test_start_time>/`, where `<test_start_time>` is a timestamp of the form `YYYY-MM-DD@hh-mm-ss`

#### Sample Usage

Here are a few examples of how to configure the test framework. Shows below, for each use case, are the key-value pairs that you have to edit in `default.yaml`

**Run E2E on 1 fresh new instance on `us-east-1`, terminated upon completion**

Here, besides the three required arguments, we have to fill-in the region_name, set `use_two_instances` to `false` to ensure that only one instance is used, leave `existing_server_instance_id` and `existing_client_instance_id` blank to create a new instance (as opposed to reusing an existing one), and set `leave_instances_on` to `false` since we want the new instance to be terminated upon completion.

```yaml
ssh_key_name: <yourname-key>
ssh_key_path: </path/to/yourname-key.pem>
github_token: <your-github-token-here>
region_name: us-east-1
use_two_instances: "false"
existing_server_instance_id:
existing_client_instance_id:
skip_host_setup: "false"
skip_git_clone: "false"
cmake_build_type: <dev, metrics or prod>
network_conditions: normal
testing_urls: https://whist-test-assets.s3.amazonaws.com/SpaceX+Launches+4K+Demo.mp4
testing_time: 126 # Length of the video in link above is 2mins, 6seconds
simulate_scrolling: 0
leave_instances_on: "false"
```

**Run E2E on 2 new instances on one among ANY REGION available, simulate scrolling during test, and leave instances on upon completion**

Compared to previous example, here we leave `region_name` blank to allow the E2E to create instances on any region available. We also set `use_two_instances: "true"` since we want to use 2 instances, and pass a non-zero number to `simulate_scrolling` to enable the scrolling test. In the example below, we also use a different website (`https://docs.nvidia.com/video-technologies/video-codec-sdk/nvenc-video-encoder-api-prog-guide/`), which is better suited for scrolling than the video from the previous example, but any url can be used.

```yaml
ssh_key_name: <yourname-key>
ssh_key_path: </path/to/yourname-key.pem>
github_token: <your-github-token-here>
region_name:
use_two_instances: "true"
existing_server_instance_id:
existing_client_instance_id:
skip_host_setup: "false"
skip_git_clone: "false"
cmake_build_type: <dev, metrics or prod>
network_conditions: normal
testing_urls: https://docs.nvidia.com/video-technologies/video-codec-sdk/nvenc-video-encoder-api-prog-guide/
testing_time: 126 # Length of the video in link above is 2mins, 6seconds
simulate_scrolling: 10
leave_instances_on: "true"
```

**Run on your development instance on `us-west-1` with degraded network** (Bandwidth: variable between 15Mbit and 30Mbit, Delay: 10 ms, Packet Drops: None, Queue Limit: None, Conditions change over time? Yes, frequency is variable between 1000 ms and 2000 ms)**, and leave instance on upon completion**

Here, the main changes compared to the previous exampels is that we are reusing an existing instance, so we need to pass the region name where the instance is located (`us-west-1`), and pass the instance ID to the `existing_server_instance_id`. We can leave the `existing_client_instance_id` blank since we are using the same instance for the server and the client (in fact, we set `use_two_instances: "false"`). To save time, because we are reusing an existing instance, we ask the E2E to skip the host-setup, since we have (hopefully) already run that on the dev instance before. We do that by setting `skip_host_setup: "true"`. If any changes were made to the host-setup script recently, we should instead set `skip_host_setup: "false"`. Finally, we pass the desired network conditions, and we ask that the instance is left on (instead of being stopped) upon completion with `leave_instances_on: "true"`

```yaml
ssh_key_name: <yourname-key>
ssh_key_path: </path/to/yourname-key.pem>
github_token: <your-github-token-here>
region_name: us-west-1
use_two_instances: "false"
existing_server_instance_id: <your-dev-instance-id>
existing_client_instance_id:
skip_host_setup: "true"
skip_git_clone: "false"
cmake_build_type: <dev, metrics or prod>
network_conditions: 15Mbit-30Mbit,10,None,None,1000-2000
testing_urls: https://whist-test-assets.s3.amazonaws.com/SpaceX+Launches+4K+Demo.mp4
testing_time: 126 # Length of the video in link above is 2mins, 6seconds
simulate_scrolling: 0
leave_instances_on: "true"
```

#### Creating New Experiments & Network Conditions

The E2E test framework can run arbitrarily many experiments for any given set of network conditions. For more details on how to set the network conditions for an experiment, check out file [apply_network_conditions.sh](helpers/setup/apply_network_conditions.sh).

You can see the list of all the experiments we run, and their network conditions, in the [`protocol-e2e-streaming-testing.yml`](../../.github/workflows/protocol-e2e-streaming-testing.yml), and you can add your new experiments there if you believe they are representative of a network condition we are not currently testing in CI.

The E2E test is also used by the [`backend-integration-test.yml`](../../.github/workflows/backend-integration-test.yml) workflow, which currently does not apply any artificial network conditions on the client instance.

### Development

The code for the E2E test framework is shared between this folder (general components) and the `.github/workflows/helpers/protocol` folders (CI-specific components). When contributing to this project, please keep your work scoped to the right folders and keep the code simple and abstracted. Note that we sometimes modify protocol logs and modify/add network experiments, so please try to keep your work as abstracted as possible to keep the documentation and code in sync.

#### E2E Test Instances

When running the E2E locally, any new AWS EC2 instance that is created will be named `manual-e2e-test-<BRANCH NAME>`, and will be accessible using the SSH key pair that you pass as a parameter.

When the E2E is run in CI, new instances will be named either `protocol-e2e-benchmarking-<BRANCH NAME>` (when the [`protocol-e2e-streaming-testing.yml`](../../.github/workflows/protocol-e2e-streaming-testing.yml) workflow is run) or `backend-integration-test-<BRANCH NAME>` (when the [`backend-integration-test.yml`](../../.github/workflows/backend-integration-test.yml) workflow is run). In either case, the instances will be accessible using the key name `GITHUB_ACTIONS_E2E_PERFORMANCE_TEST_SSH_KEYPAIR` and the corresponding certificate `GITHUB_ACTIONS_E2E_PERFORMANCE_TEST_SSH_KEYPAIR.pem`, which is stored on AWS S3 in the `whist-dev-secrets/E2E_test_secrets/` folder.

### Deployment

The test gets run automatically via the GitHub Actions workflow `protocol-e2e-streaming-testing.yml` nightly at midnight UTC for the `dev` branch, and for all PRs to `dev`, for the originating branch. The results are published as a comment to the originating PR, or to Slack in the case of the nightly runs.
