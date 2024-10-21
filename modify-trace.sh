#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <trace.json> <executable>"
    exit 1
fi

# Assign parameters to variables
trace_file="$1"
executable="$2"

# Create a temporary file for the updated JSON
temp_updated_file=$(mktemp)

# Read the original JSON, process each trace event, and write to the temporary file
jq -c '.traceEvents[]' "$trace_file" | while read -r event; do
    # Extract the memory address (name)
    name=$(echo "$event" | jq -r '.name')

    # Resolve the name using addr2line
    resolved_info=$(addr2line -f -C -e "$executable" "$name")

    # Check if addr2line returned valid output
    if [[ $? -eq 0 ]]; then
        # Split resolved_info into function name and file/line number
        func_name=$(echo "$resolved_info" | head -n 1)
        location=$(echo "$resolved_info" | tail -n 1)
        filename=$(echo "$location" | cut -d':' -f1)
        line_number=$(echo "$location" | cut -d':' -f2)

        # Create the updated event with demangled name and args
        updated_event=$(echo "$event" | jq --arg fn "$func_name" --arg fnm "$filename" --arg ln "$line_number" '
            .name = $fn |
            .args = {filename: $fnm, line: ($ln | tonumber)}
        ')
        
        # Print the updated event to the temporary file
        echo "$updated_event" >> "$temp_updated_file"
    else
        # If addr2line fails, just write the original event
        echo "$event" >> "$temp_updated_file"
    fi
done

# Create the final output JSON structure
jq -n --argfile events "$temp_updated_file" '{ traceEvents: $events }' > /tmp/updated_trace.json

# Clean up temporary files
rm "$temp_updated_file"

echo "Updated trace information has been saved to /tmp/updated_trace.json"
