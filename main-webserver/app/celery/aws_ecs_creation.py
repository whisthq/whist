from app import *
from app.helpers.utils.azure.azure_resource_creation import *



def preprocess_task_info(taskinfo):
    #TODO:  actually write this
    return []

@celery_instance.task(bind=True)
def create_new_container(self, username, taskinfo):
    fractalLog(
        function="create_new_container",
        label="None",
        logs="Creating new container on ECS with info {}".format(taskinfo)
    )

    ecs_client = ECSClient(launch_type='EC2')
    processed_task_info_cmd, processed_task_info_entrypoint, processed_image = preprocess_task_info(taskinfo)
    ecs_client.set_and_register_task(
        processed_task_info_cmd, processed_task_info_entrypoint, imagename=processed_image, family="fractal_container",
    )
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945", ],
            "securityGroups": ["sg-036ebf091f469a23e", ],
        }
    }
    ecs_client.run_task(networkConfiguration=networkConfiguration)
    self.update_state(
        state="PENDING",
        meta={
            "msg": "Creating new container on ECS with info {}".format(taskinfo)
        },
    )
    ecs_client.spin_til_running(time_delay=2)
    curr_ip = ecs_client.task_ips.get(0, -1)
    if curr_ip == -1:
        fractalLog(
            function="create_new_container",
            label=str(username),
            logs="Error generating task with running IP",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE",
            meta={
                "msg": "Error generating task with running IP"
            },
        )

        return
    private_key = generatePrivateKey()
    fractalSQLInsert(
        table_name=resourceGroupToTable("eastus"),
        params={
            "vm_name": ecs_client.tasks[0],
            "ip": curr_ip,
            "state": "CREATING",
            "location": "eastus",
            "dev": False,
            "os": "Linux",
            "lock": True,
            "disk_name": "",
            "private_key": private_key,
        },
    )

@celery_instance.task(bind=True)
def create_new_cluster(self, instance_type, ami, min_size=1, max_size=10, region_name="us-east-2", availability_zones=None):
    fractalLog(
        function="create_new_cluster",
        label="None",
        logs=f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in region {region_name}"
    )
    ecs_client = ECSClient(launch_type='EC2', region_name=region_name)

    ecs_client.run_task(networkConfiguration=networkConfiguration)
    self.update_state(
        state="PENDING",
        meta={
            "msg": f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in region {region_name}"
        },
    )

    try: 
        launch_config_name = ecs_client.create_launch_configuration(instance_type=instance_type, ami=ami, launch_config_name=None)
        auto_scaling_group_name = ecs_client.create_auto_scaling_group(launch_config_name=launch_config_name, min_size=min_size, max_size=max_size, availability_zones=availability_zones)
        capacity_provider_name = ecs_client.create_capacity_provider(auto_scaling_group_name=auto_scaling_group_name)
        return ecs_client.create_cluster(capacity_providers=[capacity_provider_name])
    except Exception as e:
        fractalLog(
            function="create_new_cluster",
            logs=f"Encountered error: {e}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE",
            meta={
                "msg": f"Encountered error: {e}"
            },
        )
