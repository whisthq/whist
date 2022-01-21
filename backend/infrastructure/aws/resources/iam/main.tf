provider "aws" {
  region = var.region
}

resource "aws_iam_user" "whist-employees" {
  for_each = var.employee_emails
  name = each.value
}

resource "aws_iam_user" "whist-admins" {
  for_each = var.admin_emails
  name = each.value
}

resource "aws_iam_group" "whist-engineering-group" {
  name = "WhistEngineering"
}

# This will add each managed policy to the group
resource "aws_iam_group_policy_attachment" "WhistEngineering_policy" {
  group      = aws_iam_group.whist-engineering-group.name
  for_each   = var.whist-engineering-managed-policies
  policy_arn = each.key
}

resource "aws_iam_group" "mfa" {
  name = "2FA"
}

resource "aws_iam_group_policy" "mfa-policy" {
  name  = "2FA"
  group = aws_iam_group.mfa.name
  policy = jsonencode(var.mfa-policy)
}

resource "aws_iam_group" "whist-aws-admins" {
  name = "WhistAWSAdmins"
}

resource "aws_iam_group_policy_attachment" "whist-aws-admin-policy" {
  group      = aws_iam_group.whist-aws-admins.name
  for_each   = var.whist-aws-admins-managed-policies
  policy_arn = each.key
}

resource "aws_iam_user_group_membership" "employee-membership" {
  for_each = aws_iam_user.whist-employees
  user   = aws_iam_user.whist-employees[each.key].name
  groups = [aws_iam_group.whist-engineering-group.name, aws_iam_group.mfa.name]
}
