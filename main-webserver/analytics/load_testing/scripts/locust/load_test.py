import os
import time

import locust


def get_num_users_and_host():
    # extracted by looking at main() in locust source
    options = locust.argument_parser.parse_options()
    return options.num_users, options.host


TOTAL_USERS, WEB_URL = get_num_users_and_host()
ADMIN_TOKEN = os.environ["ADMIN_TOKEN"]


class LoadTestUser(locust.HttpUser):
    wait_time = locust.between(0.5, 1.0)

    current_count = 0

    def on_start(self):
        url = f"{WEB_URL}/aws_container/assign_container"
        payload = '{\r\n\t"task_definition_arn": "savvy-test-ecs-taskdef-1",\r\n    "cluster_name": "savvy-ecs-test-cluster-1",\r\n    "username": "fractal-admin@gmail.com",\r\n    "region_name": "us-east-1"\r\n}'
        headers = {"Authorization": f"Bearer {ADMIN_TOKEN}"}
        while True:
            with self.client.post(
                url=url,
                headers=headers,
                data=payload,
                catch_response=True,
            ) as resp:
                if resp.status_code == 202:
                    # save id
                    self.id = resp.json()["ID"]
                    break

                # report failure, will show in logs later
                resp.failure(f"Got status code {resp.status_code}, expected 202")
                time.sleep(0.1)

    @locust.task
    def poll_celery(self):
        url = f"{WEB_URL}/status/{self.id}"
        with self.client.get(
            url=url,
            catch_response=True,
        ) as resp:
            if resp.status_code != 200:
                resp.failure(f"Expecting 200, got {resp.status_code}")
                raise locust.exception.RescheduleTask()

        ret = resp.json()
        if ret["state"] != "SUCCESS":
            # not a failure, just need to retry
            raise locust.exception.RescheduleTask()

        self.add_to_task_count()
        # check if done
        if self.get_count_task_done() == TOTAL_USERS:
            self.environment.runner.quit()
        else:
            raise locust.exception.StopUser()

    def add_to_task_count(self):
        # this does not need to be lock-protected becuase locust uses gevent
        # and all threads run single-core
        LoadTestUser.current_count += 1

    def get_count_task_done(self):
        # this does not need to be lock-protected becuase locust uses gevent
        # and all threads run single-core
        return LoadTestUser.current_count
