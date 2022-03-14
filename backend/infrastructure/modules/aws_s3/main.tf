# ------------------------------ Buckets for user app configs ------------------------------ #

# Bucket for storing user configurations
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

# ------------------------------ Buckets for assets ------------------------------ #

# Bucket for storing Whist brand assets
resource "aws_s3_bucket" "whist-brand-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-brand-assets"

  tags = {
    Name      = "whist-brand-assets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing Whist website assets
resource "aws_s3_bucket" "whist-website-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-website-assets"

  tags = {
    Name      = "whist-website-assets"
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

# Bucket for storing protocol dependency builds
resource "aws_s3_bucket" "whist-protocol-dependencies" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-protocol-dependencies"

  tags = {
    Name      = "whist-protocol-dependencies"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing client protocol shared library
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

# Bucket for dev secrets

# Bucket for storing Whist dev secrets
resource "aws_s3_bucket" "whist-dev-secrets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-dev-secrets"

  tags = {
    Name      = "whist-dev-secrets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing Terraform state files.
resource "aws_s3_bucket" "whist-terraform-state" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-terraform-state"

  tags = {
    Name      = "whist-terraform-state"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Bucket policy attachments ------------------------------ #

resource "aws_s3_bucket_policy" "whist-website-assets-policy-attachment" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-website-assets[0].id
  policy = data.aws_iam_policy_document.whist-website-assets-policy[0].json
}
