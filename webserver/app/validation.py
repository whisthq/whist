#!/usr/bin/env python3
from pydantic import BaseModel


# With pydantic, we can define our query/body datastructures with
# "one-time use" classes like this. They need to inherit from the pydantic
# BaseModel, but no other boilerplate after that. The type signatures are
# fully mypy compatible.
class MandelboxAssignBody(BaseModel):
    region: str
    username: str
    client_commit_hash: str


# flask-pydantic ships a decorator called validate().
# When validate() is used, flask-pydantic can populate the arguments to your
# route function automatically.
#
# It's important to note that flask-pydantic actually uses the name of the
# parameter to decide which data to pass to it. So "body" must be used to
# access the request body, "query" must be used to access the query parameters.
# It must also have a type signature of a class thats inherited from BaseModel.
#
# That's it! No more boilerplate, the rest of the validation comes for free.
#
# If any of the required parameters to MandelboxAssignBody are missing,
# the response will automatically 400 BAD REQUEST, and the response json
# will be something like the following:
#
# {
#    'validation_error': {
#        'query_params': [
#           {
#             'loc': ['username'],
#             'msg': ['field required'],
#             'type': ['value_error.missing'],
#           }
#        ]
#    }
# }
