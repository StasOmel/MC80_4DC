#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple FS Delete Log Parser and Plotter
Parse LittleFS and FileX delete test logs and generate performance graphs
"""

import re
import matplotlib.pyplot as plt
import numpy as np
import os

# Default log file path
DEFAULT_LOG_FILE = 'FileX_delete.log'

def parse_log_file(filename):
    """Parse delete log file and return lists of data"""
    # Regex patterns to match both LittleFS and FileX delete log lines
    # LittleFS: Deleting /test_001.bin... deleted:   1667 us, speed:  5998 KB/s
    # FileX: File test_001.bin: deleted:  13761 us, speed:   726 KB/s

    littlefs_pattern = re.compile(
        r'Deleting\s+/test_(\d+)\.bin\.\.\.\s+'
        r'deleted:\s*(\d+)\s+us,\s+'
        r'speed:\s*(\d+)\s+KB/s'
    )

    filex_pattern = re.compile(
        r'File\s+test_(\d+)\.bin:\s+'
        r'deleted:\s*(\d+)\s+us,\s+'
        r'speed:\s*(\d+)\s+KB/s'
    )

    file_numbers = []
    delete_times = []
    speeds = []

    try:
        with open(filename, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue

                # Try both patterns
                match = None
                if line.startswith('Deleting /test_'):
                    match = littlefs_pattern.search(line)
                elif line.startswith('File test_') and 'deleted:' in line:
                    match = filex_pattern.search(line)

                if match:
                    try:
                        file_num = int(match.group(1))
                        delete_time = int(match.group(2))
                        speed = int(match.group(3))

                        file_numbers.append(file_num)
                        delete_times.append(delete_time)
                        speeds.append(speed)

                    except ValueError as e:
                        print(f"Warning: Error parsing line {line_num}: {e}")

    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
        return None, None, None
    except Exception as e:
        print(f"Error reading file '{filename}': {e}")
        return None, None, None

    return file_numbers, delete_times, speeds

def print_statistics(file_numbers, delete_times, speeds):
    """Print statistics"""
    if not file_numbers:
        print("No data to analyze")
        return

    print(f"\n=== Statistics for {len(file_numbers)} files ===")
    print(f"{'Metric':<12} {'Min':<8} {'Max':<8} {'Avg':<8} {'StdDev':<8}")
    print("-" * 50)

    metrics = [
        ('delete_time', delete_times, 'us'),
        ('speed', speeds, 'KB/s')
    ]

    for name, values, unit in metrics:
        min_val = min(values)
        max_val = max(values)
        avg_val = np.mean(values)
        std_val = np.std(values)
        print(f"{name:<12} {min_val:<8.0f} {max_val:<8.0f} {avg_val:<8.1f} {std_val:<8.1f} {unit}")

def create_graphs(file_numbers, delete_times, speeds, log_filename):
    """Create all performance graphs for delete operations"""
    if not file_numbers:
        print("No data to plot")
        return

    # Extract filename without path and extension for title
    base_name = os.path.splitext(os.path.basename(log_filename))[0]
    title = f'{base_name} - Performance Analysis'

    # Create comprehensive figure
    fig = plt.figure(figsize=(15, 10))
    fig.suptitle(title, fontsize=18, fontweight='bold')

    # Create subplots in a 2x3 grid
    gs = fig.add_gridspec(2, 3, hspace=0.3, wspace=0.3)

    # 1. Delete time over file number
    ax1 = fig.add_subplot(gs[0, :2])
    ax1.plot(file_numbers, delete_times, 'r-', linewidth=1, alpha=0.7, label='Delete Time')
    ax1.scatter(file_numbers, delete_times, c='red', s=6, alpha=0.6)
    avg_delete = np.mean(delete_times)
    min_delete = min(delete_times)
    max_delete = max(delete_times)
    ax1.axhline(avg_delete, color='blue', linestyle='--', alpha=0.7,
                label=f'Avg: {avg_delete:.0f} μs')
    ax1.axhline(min_delete, color='green', linestyle=':', alpha=0.7,
                label=f'Min: {min_delete} μs')
    ax1.axhline(max_delete, color='orange', linestyle=':', alpha=0.7,
                label=f'Max: {max_delete} μs')
    ax1.set_title('Delete Time Over File Number')
    ax1.set_xlabel('File Number')
    ax1.set_ylabel('Time (μs)')
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # 2. Delete time histogram
    ax2 = fig.add_subplot(gs[0, 2])
    ax2.hist(delete_times, bins=30, alpha=0.7, color='red', edgecolor='black')
    ax2.axvline(avg_delete, color='blue', linestyle='--', alpha=0.7)
    ax2.set_title('Delete Time Distribution')
    ax2.set_xlabel('Time (μs)')
    ax2.set_ylabel('Count')
    ax2.grid(True, alpha=0.3)
    ax2.text(0.7, 0.9, f'Avg: {avg_delete:.0f}\nStd: {np.std(delete_times):.1f}',
             transform=ax2.transAxes, bbox=dict(boxstyle="round", facecolor='white', alpha=0.8),
             fontsize=8)

    # 3. Delete speed over file number
    ax3 = fig.add_subplot(gs[1, :2])
    ax3.plot(file_numbers, speeds, 'purple', linewidth=1, alpha=0.7, label='Delete Speed')
    ax3.scatter(file_numbers, speeds, c='purple', s=6, alpha=0.6)
    avg_speed = np.mean(speeds)
    min_speed = min(speeds)
    max_speed = max(speeds)
    ax3.axhline(avg_speed, color='blue', linestyle='--', alpha=0.7,
               label=f'Avg: {avg_speed:.0f} KB/s')
    ax3.axhline(min_speed, color='orange', linestyle=':', alpha=0.7,
               label=f'Min: {min_speed} KB/s')
    ax3.axhline(max_speed, color='green', linestyle=':', alpha=0.7,
               label=f'Max: {max_speed} KB/s')
    ax3.set_title('Delete Speed Over File Number')
    ax3.set_xlabel('File Number')
    ax3.set_ylabel('Speed (KB/s)')
    ax3.grid(True, alpha=0.3)
    ax3.legend()

    # 4. Speed histogram
    ax4 = fig.add_subplot(gs[1, 2])
    ax4.hist(speeds, bins=30, alpha=0.7, color='purple', edgecolor='black')
    ax4.axvline(avg_speed, color='blue', linestyle='--', alpha=0.7)
    ax4.set_title('Speed Distribution')
    ax4.set_xlabel('Speed (KB/s)')
    ax4.set_ylabel('Count')
    ax4.grid(True, alpha=0.3)
    ax4.text(0.7, 0.9, f'Avg: {avg_speed:.0f}\nStd: {np.std(speeds):.1f}',
             transform=ax4.transAxes, bbox=dict(boxstyle="round", facecolor='white', alpha=0.8),
             fontsize=8)

    # Add trend analysis for both metrics
    if len(file_numbers) > 20:
        # Add moving averages for better trend visualization
        window_size = min(50, len(delete_times) // 10)
        if window_size > 1:
            # Delete time moving average
            moving_avg_time = np.convolve(delete_times, np.ones(window_size)/window_size, mode='valid')
            ma_x = file_numbers[window_size-1:]
            ax1.plot(ma_x, moving_avg_time, 'black', linewidth=2, alpha=0.8,
                    label=f'Moving Avg ({window_size})')
            ax1.legend()

            # Speed moving average
            moving_avg_speed = np.convolve(speeds, np.ones(window_size)/window_size, mode='valid')
            ax3.plot(ma_x, moving_avg_speed, 'black', linewidth=2, alpha=0.8,
                    label=f'Moving Avg ({window_size})')
            ax3.legend()

    # Save and show
    base_name = os.path.splitext(os.path.basename(log_filename))[0]
    output_file = f'{base_name}_analysis.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\nGraphs saved to: {output_file}")

    plt.show()

def main():
    """Main function"""
    import sys

    # Get log file from command line argument or use default
    if len(sys.argv) > 1:
        log_file = sys.argv[1]
    else:
        log_file = DEFAULT_LOG_FILE

    print("FS Delete Log Parser")
    print("====================")
    print("Supports both LittleFS and FileX delete logs")

    # Check if log file exists
    if not os.path.exists(log_file):
        print(f"Error: Log file '{log_file}' does not exist")
        input("Press Enter to exit...")
        return 1

    # Parse log file
    print(f"Parsing log file: {log_file}")
    file_numbers, delete_times, speeds = parse_log_file(log_file)

    if not file_numbers:
        print("No valid entries found in log file")
        input("Press Enter to exit...")
        return 1

    print(f"Successfully parsed {len(file_numbers)} entries")

    # Show statistics
    print_statistics(file_numbers, delete_times, speeds)

    # Calculate additional insights
    if len(file_numbers) > 10:
        print(f"\n=== Additional Analysis ===")

        # Performance degradation analysis
        first_10_avg = np.mean(delete_times[:10])
        last_10_avg = np.mean(delete_times[-10:])
        degradation = ((last_10_avg - first_10_avg) / first_10_avg) * 100
        print(f"Performance change: {degradation:+.1f}% (first 10 vs last 10 files)")

        # Speed analysis
        first_10_speed = np.mean(speeds[:10])
        last_10_speed = np.mean(speeds[-10:])
        speed_change = ((last_10_speed - first_10_speed) / first_10_speed) * 100
        print(f"Speed change: {speed_change:+.1f}% (first 10 vs last 10 files)")

        # Find outliers (times > 2 std dev from mean)
        mean_time = np.mean(delete_times)
        std_time = np.std(delete_times)
        outliers = [i for i, t in enumerate(delete_times) if abs(t - mean_time) > 2 * std_time]
        if outliers:
            print(f"Outliers found: {len(outliers)} files with unusual delete times")
            print(f"Outlier file numbers: {[file_numbers[i] for i in outliers[:5]]}" +
                  ("..." if len(outliers) > 5 else ""))

    # Create graphs
    print("\nGenerating performance graphs...")
    create_graphs(file_numbers, delete_times, speeds, log_file)

    print("\nAnalysis complete!")
    input("Press Enter to exit...")
    return 0

if __name__ == "__main__":
    main()
