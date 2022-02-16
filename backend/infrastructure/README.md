# Whist Public Cloud Infrastructure

The directory contains the source code for Whist's infrastructure as defined
by Terraform.

The Whist infrastructure Terraform defines all configuration required to enable a specific public cloud region for both development and running users, except from the actual EC2 instances powering user's cloud browsers. This is handled specifically by the Whist backend system, which is responsible for handling compute and scaling for users.

For a more detailed outline of resources covered, their configurations, and the design justifications
please visit the [design spec](https://www.notion.so/whisthq/Consolidating-Infrastructure-via-Terraform-Spec-703f6c60a64a472dbc7e64ed3f40913d)

Each directory within `infrastructure/` (at the time of writing,
there is only `aws`) contains the terraform configuration for the Whist
infrastructure for that cloud provider.

For example, `aws` contains the Whist AWS infrastructure.

NOTE: `global` in this subrepo refers to the definition of `global` in AWS. If
a resource is global, it is meant that a single declaration of said resource will
span all regions within an AWS account (such as IAM or S3 namespace).

# Setup

To use, first install and setup terraform as described in [their setup
documentation](https://learn.hashicorp.com/tutorials/terraform/install-cli).

# High level overview

First, in `infrastructure/aws/resources`, we define the configuration of the actual resources each region will use. If one wants to add a new security group for all regions,
a new S3 bucket for all regions, etc then you will want to add it there.

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

## How do I add support for a new region?

If the infrastructure for the new region is the same as `common`, then
all that is necessary is to define the module with a source as `./regions/common`
and the `region` parameter being the new region to deploy to.

An example where one would like to deploy to a region called `blah-1` is shown below:

```
module "blah-1-infra" {
  source    = "./regions/common"
  region    = "blah-1"
  vpc-cidrs = var.vpc_cidr
}

```

Furthermore, say you want to add support for multiple regions. Then, you would
do the above (of course, replacing "blah-1" with an actual region), only multiple times.

For example, to deploy to both regions "blah-1" and "blah-2" with the same
infrastructure in `common` then do the below:

```
module "blah-1-infra" {
  source    = "./regions/common"
  region    = "blah-1"
  vpc-cidrs = var.vpc_cidr
}

module "blah-2-infra" {
  source    = "./regions/common"
  region    = "blah-2"
  vpc-cidrs = var.vpc_cidr
}

```

If the infrastructure to the new region is unqiue, you will need to create
a new folder in the `regions` directory and then instaniate said
new module in `main.tf` in the infrastructure root directory.

After all of that, run `terraform apply` to deploy the infrastructure to the new region.
