resource "aws_s3_bucket" "chromium-macos-arm64" {
  bucket = format("whist-chromium-macos-arm64-%s", var.env)
  
  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
}

resource "aws_s3_bucket" "chromium-macos" {
  bucket = format("whist-chromium-macos-%s", var.env)
  
  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
}

resource "aws_s3_bucket" "windows-macos" {
  bucket = format("whist-chromium-windows-%s", var.env)
  
  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
}

resource "aws_s3_bucket" "user-config" {
  bucket = format("whist-user-app-configs-%s", var.env)
  
  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
}