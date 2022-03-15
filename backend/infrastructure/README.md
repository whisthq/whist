# Whist Public Cloud Infrastructure

The directory contains the source code for Whist's infrastructure as defined by Terraform.

The Whist Terraform infrastructure defines all configuration required to enable a specific public cloud region for both development and running users, except from the actual EC2 instances powering users' cloud browsers. This is handled specifically by the Whist backend system, which is responsible for handling compute and scaling for users. The Whist backend
is specifically designed for responsiveness and scalability, and handles cloud resources which change dynamically (i.e. scaling EC2 instances up and down based on demand), while the Terraform configuration is designed for static and security-related configuration, and handles cloud resources which must remain constant (and consistant) across cloud regions.

## Development

To use, first install and setup Terraform as described in [their setup documentation](https://learn.hashicorp.com/tutorials/terraform/install-cli).

### Overview

Each directory within `infrastructure/` contains the Terraform configuration for the Whist infrastructure for a specific Whist environment (i.e. `dev`, etc.). At the time of writing, each environment only supports `aws`, but more cloud providers are planned to be supported in the future. Global cloud resources should go in `/modules`, while per-environment cloud resources should go in either `/dev`, `/staging` or `/prod`.

NOTE: `global` in this project refers to the definition of `global` in AWS. If a resource is global, it means that a single declaration of said resource will span all regions within an AWS account (such as IAM or S3 namespaces). IAM roles for Whist engineers are handeld manually, and are not included in this Terraform configuration.

Terraform must keep a state file tracked somewhere. Our Terraform state is stored in an AWS S3 bucket called `whist-terraform-state`.

### Adding New Resources

When adding new public cloud resources, make sure to add them per-environment, and apply them per-environment in your code (when applicable). You can follow the existing structure for defining Terraform variables. If you need to add a new resource type or a new public cloud, please follow the structure of `public-cloud_resource/` unless the resource is global and can be built in to `/modules`. Note that **all** public cloud resources should be defined in Terraform, so that we can easily keep track of what the resources are, what they are used for, and who created them.

Once you are done, you can check for Terraform code formatting with `terraform fmt -check` and you can validate your Terraform configuration with `terraform validate`. Lastly, you can use `terraform plan` to get a list of changes that will be made to the infrastructure.

If you need to add support for a new region within an existing cloud provider, follow the module definitions in `main.tf` within a specific environment. Note that it is **essential** for Terraform to be consistent across regions, as we make assumptions that deploy and development resources will be named analogously across regions. Since non-compute resources are usually very cheap to create, we recommend automatically enabling **all** regions when adding a new cloud provider.

## Publishing

NOTE: You should **NEVER** manually deploy the Terraform infrastructure to Whist. We deploy Terraform as part of our CI pipeline using the `whist-build-and-deploy.yml` file. You can test deploying changes within your own AWS personal account or within a test environment by running `terraform apply` once you're done with your changes.
