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
import sqlalchemy as db
from sqlalchemy.sql import text
import numpy as np
from jose import jwt
from flask_cors import CORS
from flask_mail import Mail, Message
from datetime import timedelta, datetime as dt
import dateutil.parser
import stripe
from multiprocessing.util import register_after_fork
from flask_jwt_extended import *
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail as SendGridMail
import logging
import socket
from logging.handlers import SysLogHandler
from functools import wraps
import boto3
from botocore.exceptions import NoCredentialsError
from google_auth_oauthlib.flow import Flow
import pandas as pd

load_dotenv()
