
from app.models import db

import something
from app.models import db, InstanceInfo

### Database functions
def db_container_reset(instance_id, num):
    pass

def db_container_increment(instance_id):
    pass

def db_container_decrement(instance_id):
    pass

def db_instance_add(instance_id):
    pass

def db_instance_remove(instance_id):
    pass

def choose_instance(region):
    instance = pass # ...choose_instance logic...
    db_instance_add(instance.id)
    return instance


### Endpoints for host service to hit
@route("/event_instance_down", methods=("POST",))
def event_instance_down(**kwargs):
    container_id = kwargs.get("container_id")
    db_instance_remove(instance.id)
    return "OK"

@route("/event_container_down", methods=("POST",))
def event_container_down(**kwargs):
    container_id = kwargs.get("instance_id")
    db_container_decrement(instance.id)
    return "OK"

@route("/event_container_hearbeat", methods=("POST",))
def event_container_heartbeat(**kwargs):
    instance_id = kwargs.get("instance_id")
    containers = kwargs.get("containers")
    db_container_reset(instance_id, len(containers))
    return "OK"

### Endpoints for client app to hit
@route("/container_assign", methods=("POST",))
def container_assign(**kwargs):
    region = kwargs.get("region")
    instance = choose_instance(region)
    db_container_increment(instance.id)
    return jsonify({"ID": instance.id}), ACCEPTED
