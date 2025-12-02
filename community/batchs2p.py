#!/usr/bin/env python3
"""
batchs2p.py - Batch processor for .scad files using s2p.py

This script searches for .scad files in a directory (and subdirectories) and
processes each file by calling s2p.py with the filename as parameter.
All output is logged to logbatchs2p.txt.
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path
from datetime import datetime


def find_scad_files(root_dir, max_depth):
    """
    Find all .scad files in root_dir up to max_depth levels deep.

    Args:
        root_dir: Starting directory path
        max_depth: Maximum depth to search (0 = only root_dir)

    Returns:
        List of Path objects for found .scad files
    """
    scad_files = []
    root_path = Path(root_dir).resolve()

    for dirpath, dirnames, filenames in os.walk(root_path):
        # Calculate current depth
        current_path = Path(dirpath)
        try:
            relative = current_path.relative_to(root_path)
            depth = len(relative.parts)
        except ValueError:
            depth = 0

        # Stop going deeper if we've exceeded max_depth
        if depth > max_depth:
            dirnames.clear()  # Don't descend into subdirectories
            continue

        # Find .scad files in current directory
        for filename in filenames:
            if filename.lower().endswith('.scad'):
                scad_files.append(current_path / filename)

    return sorted(scad_files)


def process_file(scad_file, log_file):
    """
    Process a single .scad file by calling s2p.py.

    Args:
        scad_file: Path to the .scad file
        log_file: File handle for logging

    Returns:
        True if successful, False if error occurred
    """
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log_file.write(f"\n{'='*80}\n")
    log_file.write(f"[{timestamp}] Processing: {scad_file}\n")
    log_file.write(f"{'='*80}\n")
    log_file.flush()

    try:
        # Call s2p.py with the .scad file as parameter
        result = subprocess.run(
            ['python', 's2p.py', str(scad_file)],
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout per file
        )

        # Write stdout and stderr to log
        if result.stdout:
            log_file.write(result.stdout)
        if result.stderr:
            log_file.write(f"STDERR:\n{result.stderr}\n")

        log_file.flush()

        # Check return code
        if result.returncode == 0:
            log_file.write(f"✓ SUCCESS: {scad_file}\n")
            return True
        else:
            log_file.write(f"✗ ERROR: {scad_file} (exit code: {result.returncode})\n")
            return False

    except subprocess.TimeoutExpired:
        log_file.write(f"✗ TIMEOUT: {scad_file}\n")
        log_file.flush()
        return False
    except Exception as e:
        log_file.write(f"✗ EXCEPTION: {scad_file} - {str(e)}\n")
        log_file.flush()
        return False


def main():
    parser = argparse.ArgumentParser(
        description='Batch process .scad files with s2p.py',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        'folder',
        help='Root folder to search for .scad files'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Only show found files, do not process them'
    )

    parser.add_argument(
        '--depth',
        type=int,
        default=9,
        help='Maximum directory depth to search (0=only given folder, default=9)'
    )

    parser.add_argument(
        '--error',
        type=int,
        choices=[0, 1],
        default=0,
        help='Error handling: 1=stop on first error, 0=continue (default=0)'
    )

    args = parser.parse_args()

    # Validate folder
    if not os.path.isdir(args.folder):
        print(f"Error: '{args.folder}' is not a valid directory", file=sys.stderr)
        sys.exit(1)

    # Find all .scad files
    print(f"Searching for .scad files in '{args.folder}' (depth={args.depth})...")
    scad_files = find_scad_files(args.folder, args.depth)

    print(f"\nFound {len(scad_files)} .scad file(s):")
    for f in scad_files:
        print(f"  - {f}")

    if len(scad_files) == 0:
        print("\nNo .scad files found.")
        return

    # Dry-run mode: just show files and exit
    if args.dry_run:
        print("\n--dry-run mode: no files will be processed")
        return

    # Process files
    print(f"\nProcessing files (error mode: {'stop on error' if args.error == 1 else 'continue'})...")

    success_count = 0
    error_count = 0

    with open('logbatchs2p.txt', 'w', encoding='utf-8') as log_file:
        log_file.write(f"Batch S2P Processing Log\n")
        log_file.write(f"Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        log_file.write(f"Root folder: {args.folder}\n")
        log_file.write(f"Max depth: {args.depth}\n")
        log_file.write(f"Total files: {len(scad_files)}\n")
        log_file.write(f"\n")

        for i, scad_file in enumerate(scad_files, 1):
            print(f"\n[{i}/{len(scad_files)}] Processing: {scad_file.name}")

            success = process_file(scad_file, log_file)

            if success:
                success_count += 1
                print(f"  ✓ Success")
            else:
                error_count += 1
                print(f"  ✗ Error")

                if args.error == 1:
                    print("\n--error=1: Stopping due to error")
                    log_file.write(f"\n{'='*80}\n")
                    log_file.write("STOPPED: Error encountered with --error=1\n")
                    break

        # Write summary
        log_file.write(f"\n{'='*80}\n")
        log_file.write(f"SUMMARY\n")
        log_file.write(f"{'='*80}\n")
        log_file.write(f"Total files found: {len(scad_files)}\n")
        log_file.write(f"Successfully processed: {success_count}\n")
        log_file.write(f"Errors: {error_count}\n")
        log_file.write(f"Completed: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")

    # Print summary
    print(f"\n{'='*80}")
    print(f"SUMMARY")
    print(f"{'='*80}")
    print(f"Total files found: {len(scad_files)}")
    print(f"Successfully processed: {success_count}")
    print(f"Errors: {error_count}")
    print(f"\nLog saved to: logbatchs2p.txt")


if __name__ == '__main__':
    main()
