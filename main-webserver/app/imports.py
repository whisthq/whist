import os
import random
import string
import time
import sys
import logging
import requests
import os.path
import traceback
import stripe
import datetime
import pytz
import logging
import socket
import boto3
import dateutil.parser
import sqlalchemy as db
import numpy as np

from dateutil.relativedelta import relativedelta
from inspect import getsourcefile
from sqlalchemy.orm import sessionmaker
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
from sqlalchemy.sql import text
from jose import jwt
from flask_cors import CORS
from flask_mail import Mail, Message
from datetime import timedelta, datetime as dt
from multiprocessing.util import register_after_fork
from flask_jwt_extended import *
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail as SendGridMail
from logging.handlers import SysLogHandler
from functools import wraps
from botocore.exceptions import NoCredentialsError
from msrest.exceptions import ClientException


load_dotenv()
