# ------------------------------ Buckets for user app configs ------------------------------ #

# Buckets for storing user configurations, per environment
resource "aws_s3_bucket" "whist-user-app-configs" {
  bucket = "whist-user-app-configs-${var.env}"

  # Ignore the internal lifecycle rules because we have
  # a separate lifecycle resource.
  # See: https://registry.terraform.io/providers/hashicorp/aws/latest/docs/resources/s3_bucket_lifecycle_configuration#usage-notes
  lifecycle {
    ignore_changes = [
      lifecycle_rule
    ]
  }

  tags = {
    Name      = "whist-user-app-configs-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for Whist assets ------------------------------ #

# Bucket for storing general Whist brand assets
resource "aws_s3_bucket" "whist-brand-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-brand-assets"

  tags = {
    Name      = "whist-brand-assets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing Whist test assets
resource "aws_s3_bucket" "whist-test-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-test-assets"

  tags = {
    Name      = "whist-test-assets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing fonts used in Whist
resource "aws_s3_bucket" "whist-fonts" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-fonts"

  tags = {
    Name      = "whist-fonts"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for protocol ------------------------------ #

# Bucket for storing protocol E2E test logs
resource "aws_s3_bucket" "whist-e2e-protocol-test-logs" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-e2e-protocol-test-logs"

  tags = {
    Name      = "whist-e2e-protocol-test-logs"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing protocol build dependencies binaries (i.e. shared libraries)
resource "aws_s3_bucket" "whist-protocol-dependencies" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-protocol-dependencies"

  tags = {
    Name      = "whist-protocol-dependencies"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing the client protocol shared library (for external bundling, like Chromium)
resource "aws_s3_bucket" "whist-protocol-client-shared-lib" {
  count  = var.env == "dev" ? 1 : 0
  bucket = "whist-protocol-client-shared-lib"

  tags = {
    Name      = "whist-protocol-client-shared-lib-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for other data ------------------------------ #

# Bucket for storing Whist development secrets (Apple codesigning files, etc.)
resource "aws_s3_bucket" "whist-dev-secrets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-dev-secrets"

  tags = {
    Name      = "whist-dev-secrets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing Terraform state files
resource "aws_s3_bucket" "whist-terraform-state" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-terraform-state"

  tags = {
    Name      = "whist-terraform-state"
    Env       = var.env
    Terraform = true
  }
}
