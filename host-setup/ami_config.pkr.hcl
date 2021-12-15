/*
 * This Packer HCL configuration file creates an AWS EBS-backed AMI. For full documentation,
 * pleaser refer to this link: https://www.packer.io/docs/builders/amazon/ebs
 */

/* 
 * Variables passed to Packer via the -var-file flag, as a JSON file.
 */

/* AWS variables */

variable "access_key" {
  type      = string
  default   = ""
  sensitive = true
}

variable "secret_key" {
  type      = string
  default   = ""
  sensitive = true
}

variable "instance_type" {
  type    = string
  default = ""
}

variable "ami_name" {
  type    = string
  default = ""
}

variable "source_ami" {
  type    = string
  default = ""
}

variable "source_region" {
  type    = string
  default = ""
}

variable "destination_regions" {
  type    = list(string)
  default = []
}

variable "vpc_id" {
  type    = string
  default = ""
}

/* GitHub variables */

variable "git_branch" {
  type    = string
  default = ""
}

variable "git_hash" {
  type    = string
  default = ""
}

variable "pr_number" {
  type    = string
  default = ""
}

variable "github_pat" {
  type      = string
  default   = ""
  sensitive = true
}

variable "github_username" {
  type      = string
  default   = ""
  sensitive = true
}

/* Miscellaneous variables */

variable "mandelbox_logz_shipping_token" {
  type    = string
  default = ""
}

/* 
 * Packer Builder configuration, using the variables from the `variable` configurations defined above.
 * Note that we don't specify availability_zone so that Packer tries all availabilities zones it is configured
 * for (i.e. zones with a subnet with tag `Purpose: packer`) in the `region`. 
 */

source "amazon-ebs" "Whist_AWS_AMI_Builder" {

  /* AWS AMI Packer parameters */

  ami_description = "Whist-optimized Ubuntu 20.04 AWS Machine Image"
  ami_virtualization_type = "hvm" # Options are paravirtual (default) or hvm. AWS recommends usings HVM, as per: https://cloudacademy.com/blog/aws-ami-hvm-vs-pv-paravirtual-amazon/
  ami_name        = "${var.ami_name}"
  ami_regions     = "${var.destination_regions}" # All AWS regions the AMI will get copied to
  region          = "${var.source_region}" # The source AWS region where the Packer Builder will run
  source_ami      = "${var.source_ami}"
  instance_type   = "${var.instance_type}"




  ena_support           = true
  # Enable enhanced networking (ENA but not SriovNetSupport) on HVM-compatible AMIs. If set, add ec2:ModifyInstanceAttribute to your AWS IAM policy.
  # Note: you must make sure enhanced networking is enabled on your instance. See Amazon's documentation on enabling enhanced networking.
  sriov_support         = true 
  # Enable enhanced networking (SriovNetSupport but not ENA) on HVM-compatible AMIs. If true, add ec2:ModifyInstanceAttribute to your AWS IAM policy. Note: you must make sure enhanced networking is enabled on your instance. See Amazon's documentation on enabling enhanced networking. Default false.


  force_deregister      = true # Force Packer to first deregister an existing AMI if one with the same name already exists
  force_delete_snapshot = true # Force Packer to delete snapshots associated with AMIs, which have been deregistered by force_deregister



  /* AWS VPC Packer parameters */


  vpc_id          = "${var.vpc_id}" # The VPC where the Packer Builer will run. This VPC needs to have subnet(s) configured as per the `subnet_filter` below


  ssh_username = "ubuntu"

associate_public_ip_address = true  # (bool) - If using a non-default VPC, public IP addresses are not provided by default. If this is true, your new instance will get a Public IP. default: false

availability_zone (string) - Destination availability zone to launch instance in. Leave this empty to allow Amazon to auto-assign.



  subnet_filter {
    filters = {
      "tag:Purpose": "packer"
    }
    most_free = true # The Subnet with the most free IPv4 addresses will be used if multiple Subnets matches the filter.
    random    = true # A random Subnet will be used if multiple Subnets matches the filter. most_free have precendence over this.
  }


  max_retries = 5 # This is the maximum number of times an API call is retried, in the case where requests are being throttled or experiencing transient failures. The delay between the subsequent API calls increases exponentially.



  /* AWS IAM Packer parameters */

  access_key   = "${var.access_key}"
  secret_key   = "${var.secret_key}"
  iam_instance_profile = "PackerAMIBuilder" # This is the IAM role configured for Packer in AWS

  /* AWS EBS Packer parameters */

  ebs_optimized = true
  launch_block_device_mappings {
    delete_on_termination = true # This ensures that the EBS volume of the EC2 instance(s) using the AMI Packer creates get deleted when the instance gets deleted
    device_name           = "/dev/sda1"
    volume_size           = 64
    volume_type           = "gp3" # Options are gp2 and gp3. gp3 is the newer, more performant and cheaper AWS block volume
  }



shutdown_behavior (string) - Automatically terminate instances on shutdown in case Packer exits ungracefully. Possible values are stop and terminate. Defaults to stop.







  /* AWS EC2 Tags Packer parameters */

  run_tag {
    key = "Instance Region"
    value = "${var.source_region}"
  }

  run_tag {
    key = "Git Commit SHA"
    value = "${var.git_hash}"
  }

  run_tag {
    key = "PR Number"
    value = "${var.pr_number}"
  }
}


/* 
 * Packer build commands, run on the Packer Builder created via the `source` configuration defined above. 
 */

build {
  sources = ["source.amazon-ebs.Whist_AWS_AMI_Builder"]

  provisioner "shell" {
    inline = ["while [ ! -f /var/lib/cloud/instance/boot-finished ]; do echo 'Waiting for cloud-init...'; sleep 1; done"]
  }

  provisioner "shell" {
    inline = ["sudo systemctl disable --now unattended-upgrades.service"]
  }

  provisioner "file" {
    destination  = "/home/ubuntu"
    pause_before = "10s"
    source       = "../host-setup"
  }

  provisioner "shell" {
    inline = ["sudo mkdir -p /usr/share/whist", "sudo mv /home/ubuntu/host-setup/app_env.env /usr/share/whist/app_env.env"]
  }

  provisioner "file" {
    destination = "/home/ubuntu/host-service"
    source      = "../host-service/build/host-service"
  }

  provisioner "shell" {
    inline       = ["cd /home/ubuntu/host-setup", "./setup_host.sh --deployment ${var.github_username} ${var.github_pat} ${var.git_branch} ${var.git_hash} ${var.mandelbox_logz_shipping_token}", "cd ..", "rm -rf host-setup"]
    pause_before = "10s"
  }

  post-processor "manifest" {
    output     = "manifest.json"
    strip_path = true
  }
}
