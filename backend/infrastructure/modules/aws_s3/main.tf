# ------------------------------ Buckets for macOS Whist Chromium Browser ------------------------------ #

# Bucket for storing Chromium builds for macOS (arm64)
resource "aws_s3_bucket" "whist-browser-macos-arm64" {
  bucket = "whist-browser-macos-arm64-${var.env}"

  tags = {
    Name      = "whist-browser-macos-arm64-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing Chromium builds for macOS (x64)
resource "aws_s3_bucket" "whist-browser-macos-x64" {
  bucket = "whist-browser-macos-x64-${var.env}"

  tags = {
    Name      = "whist-browser-macos-x64-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------- Bucket for Linux Server-side Whist Chromium Browser ------------------------ #

# Bucket for storing Server-side Chromium builds for Linux Ubuntu (x64)
resource "aws_s3_bucket" "whist-serverside-browser-linux-x64" {
  bucket = "whist-serverside-browser-linux-x64-${var.env}"

  tags = {
    Name      = "whist-serverside-browser-linux-x64-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for Whist assets ------------------------------ #

# Bucket for storing general Whist brand assets
resource "aws_s3_bucket" "whist-brand-assets" {
  count  = var.env == "dev" ? 1 : 0
  bucket = "whist-brand-assets"

  tags = {
    Name      = "whist-brand-assets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing Whist test assets
resource "aws_s3_bucket" "whist-test-assets" {
  count  = var.env == "dev" ? 1 : 0
  bucket = "whist-test-assets"

  tags = {
    Name      = "whist-test-assets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing fonts used in Whist
resource "aws_s3_bucket" "whist-fonts" {
  count  = var.env == "dev" ? 1 : 0
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
  count  = var.env == "dev" ? 1 : 0
  bucket = "whist-e2e-protocol-test-logs"

  tags = {
    Name      = "whist-e2e-protocol-test-logs"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing protocol build dependencies binaries (i.e. shared libraries)
resource "aws_s3_bucket" "whist-protocol-dependencies" {
  count  = var.env == "dev" ? 1 : 0
  bucket = "whist-protocol-dependencies"

  tags = {
    Name      = "whist-protocol-dependencies"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for other data ------------------------------ #

# Bucket for storing Whist development secrets (Apple codesigning files, etc.)
resource "aws_s3_bucket" "whist-dev-secrets" {
  count  = var.env == "dev" ? 1 : 0
  bucket = "whist-dev-secrets"

  tags = {
    Name      = "whist-dev-secrets"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for storing Terraform state files
resource "aws_s3_bucket" "whist-terraform-state" {
  count  = var.env == "dev" ? 1 : 0
  bucket = "whist-terraform-state"

  tags = {
    Name      = "whist-terraform-state"
    Env       = var.env
    Terraform = true
  }
}
