import os, random, string

from azure.common.credentials import ServicePrincipalCredentials
from azure.mgmt.resource import ResourceManagementClient
from azure.mgmt.network import NetworkManagementClient
from azure.mgmt.compute import ComputeManagementClient
from azure.mgmt.compute.models import DiskCreateOption
from msrestazure.azure_exceptions import CloudError
from haikunator import Haikunator
from dotenv import *
from flask_sqlalchemy import SQLAlchemy
from celery import Celery, uuid
from flask import *
import sqlalchemy as db
from sqlalchemy.sql import text
import numpy as np
from jose import jwt

load_dotenv()
haikunator = Haikunator()
