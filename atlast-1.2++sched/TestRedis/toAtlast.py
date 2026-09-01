#!/usr/bin/env python3
import os
import argparse

def convert_env_to_atl(input_filename, output_filename):
    base_name = os.path.basename(input_filename)
    
    with open(input_filename, 'r') as infile, open(output_filename, 'w') as outfile:
        outfile.write(f"0 value {base_name}\n\n")
        outfile.write(f"mk-string-map to {base_name}\n\n")
        
        for line in infile:
            line = line.strip()
            
            if not line or line.startswith('#'):
                continue
                
            if '=' in line:
                key, val = line.split('=', 1)
                outfile.write(f'{base_name} "{key.strip()}" "{val.strip()}" map-add\n')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert an environment file into an Atlast (.atl) map format."
    )
    parser.add_argument("input_file", help="Path to the input environment file")
    parser.add_argument("output_file", nargs="?", help="Path to the output .atl file")

    args = parser.parse_args()

    # If output_file was not provided on the command line, show help and exit
    if not args.output_file:
        parser.print_help()
        exit(1)

    convert_env_to_atl(args.input_file, args.output_file)
    print(f"Successfully converted {args.input_file} to {args.output_file} using base name '{os.path.basename(args.input_file)}'")
