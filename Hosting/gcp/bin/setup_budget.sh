#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_apply "${1:-}"
require_tools gcloud

budget_name="Gorilla Protocol monthly guardrail"
billing_account_name="$(gcloud billing projects describe "${GCP_PROJECT_ID}" --format='value(billingAccountName)')"
billing_account="${billing_account_name##*/}"
project_number="$(gcloud projects describe "${GCP_PROJECT_ID}" --format='value(projectNumber)')"

test -n "${billing_account}" || die "The project has no billing account."
test -n "${project_number}" || die "The project number could not be read."

existing_budget="$(gcloud billing budgets list \
  --billing-account="${billing_account}" \
  --billing-project="${GCP_PROJECT_ID}" \
  --filter="displayName='${budget_name}'" \
  --format='value(name)' | head -n 1)"

if [[ -n "${existing_budget}" ]]; then
  echo "Budget already exists: ${budget_name}"
  exit 0
fi

gcloud billing budgets create \
  --billing-account="${billing_account}" \
  --billing-project="${GCP_PROJECT_ID}" \
  --display-name="${budget_name}" \
  --budget-amount="${GCP_MONTHLY_BUDGET_USD}USD" \
  --calendar-period=month \
  --filter-projects="projects/${project_number}" \
  --credit-types-treatment=include-all-credits \
  --threshold-rule=percent=0.50 \
  --threshold-rule=percent=0.80 \
  --threshold-rule=percent=1.00 \
  --threshold-rule=percent=1.00,basis=forecasted-spend \
  --ownership-scope=billing-account

echo "Monthly net-spend budget created at USD ${GCP_MONTHLY_BUDGET_USD}."
