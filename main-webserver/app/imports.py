import os, random, string, time, sys, logging, requests
import os.path

import stripe
import datetime
import pytz
from dateutil.relativedelta import relativedelta
from inspect import getsourcefile

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

load_dotenv()
