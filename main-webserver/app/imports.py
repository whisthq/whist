import os, random, string, time

from azure.common.credentials import ServicePrincipalCredentials
from azure.mgmt.resource import ResourceManagementClient
from azure.mgmt.network import NetworkManagementClient
from azure.mgmt.compute import ComputeManagementClient
from azure.mgmt.compute.models import DiskCreateOption
from msrestazure.azure_exceptions import CloudError
from haikunator import Haikunator
from dotenv import *
from flask_sqlalchemy import *
from flask_migrate import Migrate
from celery import Celery, uuid
from flask import *
import sqlalchemy as db
from sqlalchemy.sql import text
import numpy as np
from jose import jwt
from flask_cors import CORS
from flask_mail import Mail, Message
from datetime import datetime as dt
import stripe
from multiprocessing.util import register_after_fork

load_dotenv()
haikunator = Haikunator()
