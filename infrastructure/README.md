# Whist Infrastructure

The directory contains the source code for Whist's infrastructure as defined
by Terraform.

Each directory within `fractal/infrastructure` (at the time of writing,
there is only `aws`) contains the terraform configuration for the Whist
infrastructure for that cloud provider.

For example, `aws` contains the Whist AWS infrastructure.

# Setup

To use, first install and setup terraform as described in [their setup
documentation](https://learn.hashicorp.com/tutorials/terraform/install-cli).

# High level overview

First, in `fractal/infrastructure/aws/infrastructure`, we define the configuration of the actual resources each region will use. If one wants to add a new security group for all regions,
a new S3 bucket for all regions, etc then you will want to add it there.

Next, in `fractal/infrastructure/aws/regions` defines the infrastructure used in each
region. The files in this directory will use the infrastructure defined in
`fractal/infrastructure/aws/infrastructure` to define the infrastructure for
its own region.

Finally, in `fractal/infrastructure/aws/main.tf` we import the region specific modules we defined in `fractal/infrastructure/aws/regions` and the global resources as defined
in `fractal/infrastructure/aws/infrastructure`, which will encompass our entire
infrastructure.

With that all together, we can use `terraform apply` to deploy our entire infrastructure.

# How to use

This is only for AWS. If other cloud providers are added in, please update this section.

To deploy infrastructure, naviagate to `./infrastructure/aws` and run `terraform apply`.
To destroy infrastructure, naviagate to `./infrastructure/aws` and run `terraform destroy`.
To examine what resources will be added, removed, or modified after a change to the
codebase, run `terraform plan`.

# FAQ

## How do I add or remove employees?

In `fractal/infrastructure/aws`, alter the `employee_emails` variable
in the `variables.tf` file. Add the email to add an employee. Remove said
employee name to remove them.

After the modification has been made, run `terraform apply`.

## How to I change configuration of various resources

Each core infrastructure category (buckets, IAM, networking, etc) is defined
within the `fractal/infrastructure/aws/infrastructure` directory.

Within each are the configuration files (`.tf` files) for each major resource
category. One can modify these files to change the resource for all environments
and regions.

For example, to add a new environment, say `gamma`, to all regions, add
`gamma` to the `environments` variable in the
`fractal/infrastructure/aws/infrastructure/networking/variables.tf` file and
run `terraform apply`.

## How do I add a new region?

First, add a new folder `${your new region}` to the `regions` folder.

Second, if this infrastructure is exactly the same as the others, just
copy each file from the others and paste it in your new region folder.

Lastly, import that module into the top-level `fractal/infrastructure/aws/main.tf`
file as shown for the other regions.

Now, run `terraform apply` to deploy the infrastructure to the new region.
