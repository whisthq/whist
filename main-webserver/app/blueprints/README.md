# Module Structure
At the module level, the code is separated between Azure, AWS, mail, admin, payment, auth, and celery blueprints (post and get request hendlers)

## Admin
This folder contains our admin login, logs, report generation, and admin dashboard endpoints.

## Auth
This folder contains our account generation and manipulation endpoints, plus login.

## Azure
This folder contains our Azure VM creation, deletion, modification and ping endpoints

## AWS
This folder contains our AWS container creation, deletion, modification and ping endpoints

## Mail
This folder contains our email generation and send endpoints

## Payment
This folder contains our stripe handling/payment endpoints

## Celery
This folder contains our celery task status getter endpoints.
