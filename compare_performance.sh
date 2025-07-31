#!/bin/bash

# Compare performance between Ancible and Ansible
# Usage: ./compare_performance.sh [playbook_file]

# Default playbook if not specified
PLAYBOOK=${1:-"examples/playbooks/1_simple.yml"}
INVENTORY="examples/inventory_local.ini"

echo "Comparing performance for playbook: $PLAYBOOK"
echo "----------------------------------------"

# Run Ancible and measure time
echo "Running Ancible..."
TIMEFORMAT="Ancible execution time: %Rs"
time ./bin/ancible-playbook -i $INVENTORY $PLAYBOOK
echo ""

# Run Ansible and measure time
echo "Running Ansible..."
TIMEFORMAT="Ansible execution time: %Rs"
time ansible-playbook -i $INVENTORY $PLAYBOOK
echo ""

# Print conclusion
echo "Performance comparison complete."