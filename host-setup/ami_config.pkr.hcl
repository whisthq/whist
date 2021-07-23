
variable "access_key" {
  type      = string
  default   = ""
  sensitive = true
}

variable "ami_name" {
  type    = string
  default = ""
}

variable "destination_regions" {
  type    = list(string)
  default = []
}

variable "git_branch" {
  type    = string
  default = ""
}

variable "git_hash" {
  type    = string
  default = ""
}

variable "git_commit_sha" {
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

variable "instance_type" {
  type    = string
  default = ""
}

variable "secret_key" {
  type      = string
  default   = ""
  sensitive = true
}

variable "source_ami" {
  type    = string
  default = ""
}

variable "source_region" {
  type    = string
  default = ""
}

variable "subnet_id" {
  type    = string
  default = ""
}

variable "vpc_id" {
  type    = string
  default = ""
}

source "amazon-ebs" "Fractal_AWS_AMI_Builder" {
  access_key           = "${var.access_key}"
  ami_description      = "Fractal-optimized Ubuntu 20.04 AWS Machine Image."
  ami_name             = "${var.ami_name}"
  ami_regions          = "${var.destination_regions}"
  ebs_optimized        = true
  iam_instance_profile = "PackerAMIBuilder"
  instance_type        = "${var.instance_type}"
  launch_block_device_mappings {
    delete_on_termination = true
    device_name           = "/dev/sda1"
    volume_size           = 64
    volume_type           = "gp2"
  }
  region       = "${var.source_region}"
  secret_key   = "${var.secret_key}"
  source_ami   = "${var.source_ami}"
  ssh_username = "ubuntu"
  subnet_id    = "${var.subnet_id}"
  vpc_id       = "${var.vpc_id}"
  force_deregister      = true
  force_delete_snapshot = true
  run_tag {
    key = "Name"
    value = "Packer-Builder-${var.source_region}-${var.pr_number}-${var.git_commit_sha}"
  }
}

build {
  sources = ["source.amazon-ebs.Fractal_AWS_AMI_Builder"]

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
    inline = ["sudo mkdir -p /usr/share/fractal", "sudo mv /home/ubuntu/host-setup/app_env.env /usr/share/fractal/app_env.env"]
  }

  provisioner "file" {
    destination = "/home/ubuntu/ecs-host-service"
    source      = "../ecs-host-service/build/ecs-host-service"
  }

  provisioner "shell" {
    inline = ["cd /home/ubuntu/host-setup", "./setup_ubuntu20_host.sh"]
  }

  provisioner "shell" {
    expect_disconnect = "true"
    inline            = ["sudo reboot"]
  }

  provisioner "shell" {
    inline       = ["cd /home/ubuntu/host-setup", "./setup_ubuntu20_ami_host.sh ${var.github_username} ${var.github_pat} ${var.git_branch} ${var.git_hash}", "cd ..", "rm -rf host-setup"]
    pause_before = "10s"
  }

  post-processor "manifest" {
    output     = "manifest.json"
    strip_path = true
  }
}
