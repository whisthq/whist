# ------------------------------ Buckets for Chromium builds ------------------------------ #

# Bucket for storing Chromium builds for MacOS Arm64
resource "aws_s3_bucket" "whist-chromium-macos-arm64" {
  bucket = "whist-chromium-macos-arm64-${var.env}"

  tags = {
    Name        = "whist-chromium-macos-arm64-${var.env}"
    Env         = var.env
    Terraform   = true
  }
}

# Bucket for storing Chromium builds for MacOS x64
resource "aws_s3_bucket" "whist-chromium-macos-x64" {
  bucket = "whist-chromium-macos-x64-${var.env}"

  tags = {
    Name        = "whist-chromium-macos-${var.env}"
    Env         = var.env
    Terraform   = true
  }
}

# Bucket for storing Chromium builds for Windows
resource "aws_s3_bucket" "whist-chromium-windows" {
  bucket = "whist-chromium-windows-${var.env}"

  tags = {
    Name        = "whist-chromium-windows-${var.env}"
    Env         = var.env
    Terraform   = true
  }
}

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

# Bucket for storing protocol shared libs
resource "aws_s3_bucket" "whist-protocol-shared-libs" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-protocol-shared-libs"

  tags = {
    Name      = "whist-protocol-shared-libs"
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
