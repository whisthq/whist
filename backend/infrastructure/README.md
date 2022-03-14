# Whist Public Cloud Infrastructure

The directory contains the source code for Whist's infrastructure as defined
by Terraform.

The Whist infrastructure Terraform defines all configuration required to enable a specific public cloud region for both development and running users, except from the actual EC2 instances powering user's cloud browsers. This is handled specifically by the Whist backend system, which is responsible for handling compute and scaling for users. The Whist backend
is specifically designed for responsiveness and scalability, and handles cloud resources which change dynamically (i.e. scaling EC2 instances up and down based on demand), while the Terraform configuration is designed for static and security-related configuration, and handles cloud resources which must remain constant (and consistant) across cloud regions.

## Development 

To use, first install and setup Terraform as described in [their setup
documentation](https://learn.hashicorp.com/tutorials/terraform/install-cli).

### Overview

Each directory within `infrastructure/` contains the Terraform configuration for the Whist
infrastructure for a specific Whist environment (i.e. `dev`, etc.). At the time of writing, each environment only 
supports `aws`, but more cloud providers are planned to be supported in the future. Global cloud resources should go in `/modules`, while per-environment cloud resources should go in either `/dev`, `/staging` or `/prod`.

NOTE: `global` in this project refers to the definition of `global` in AWS. If
a resource is global, it means that a single declaration of said resource will
span all regions within an AWS account (such as IAM or S3 namespaces).






### Adding other regions/cloud providers


## How do I add support for a new region?

If the infrastructure for the new region is the same as `common`, then
all that is necessary is to define the module with a source as `./regions/common`
and the `region` parameter being the new region to deploy to.




If the infrastructure to the new region is unqiue, you will need to create
a new folder in the `regions` directory and then instaniate said
new module in `main.tf` in the infrastructure root directory.

After all of that, run `terraform apply` to deploy the infrastructure to the new region.





### Styling


We use [TFLint](https://github.com/terraform-linters/tflint)



## Publishing









First, in `infrastructure/aws/resources`, we define the configuration of the actual resources each region will use. If one wants to add a new security group for all regions,
a new S3 bucket for all regions, etc., then you will want to add it there.

Next, in `infrastructure/aws/regions` defines the infrastructure used in each
region. The files in this directory will use the infrastructure defined in
`infrastructure/aws/resources` to define the infrastructure for
its own region. All regions as of now have the same infrastructure, so the common
infrastructure is developed in `common` folder.

Finally, in `infrastructure/aws/main.tf` we import the region specific modules we defined in `infrastructure/aws/regions` and the global resources as defined
in `infrastructure/aws/resources`, which will encompass our entire
infrastructure.

With that all together, we can use `terraform apply` to deploy our entire infrastructure.

# How to use

This is only for AWS. If other cloud providers are added in, please update this section.

To deploy infrastructure, naviagate to `infrastructure/aws` and run `terraform apply`.
To destroy infrastructure, naviagate to `infrastructure/aws` and run `terraform destroy`.
To examine what resources will be added, removed, or modified after a change to the
codebase, run `terraform plan`.

# FAQ

## How do I add or remove employees?

In `infrastructure/aws`, alter the `employee_emails` variable
in the `variables.tf` file. Add the email to add an employee. Remove said
employee name to remove them.

After the modification has been made, run `terraform apply`.

## How to I change configuration of various resources

Each core infrastructure category (buckets, IAM, networking, etc) is defined
within the `infrastructure/aws/resources` directory.

Within each are the configuration files (`.tf` files) for each major resource
category. One can modify these files to change the resource for all environments
and regions.

For example, to add a new environment, say `gamma`, to all regions, add
`gamma` to the `environments` variable in the
`infrastructure/aws/resources/networking/variables.tf` file and
run `terraform apply`.

