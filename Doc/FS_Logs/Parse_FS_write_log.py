#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple LittleFS Write Log Parser and Plotter
Parse LittleFS write test log and generate performance graphs
"""

import re
import matplotlib.pyplot as plt
import numpy as np
import os

# Default log file path
DEFAULT_LOG_FILE = 'LittleFS_write.log'

def parse_log_file(filename):
    """Parse write log file and return lists of data"""
    # Regex pattern to match write log lines
    # Example: Writing /test_001.bin... opened:  2689 us, closed: 244957 us, I/O:  90733 us, total: 343288 us, speed:   110 KB/s, CRC32: 0xC61C2AA0
    pattern = re.compile(
        r'Writing\s+/test_(\d+)\.bin\.\.\.\s+'
        r'opened:\s*(\d+)\s+us,\s+'
        r'closed:\s*(\d+)\s+us,\s+'
        r'I/O:\s*(\d+)\s+us,\s+'
        r'total:\s*(\d+)\s+us,\s+'
        r'speed:\s*(\d+)\s+KB/s'
    )

    file_numbers = []
    open_times = []
    close_times = []
    io_times = []
    total_times = []
    speeds = []

    try:
        with open(filename, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line or not line.startswith('Writing /test_'):
                    continue

                match = pattern.search(line)
                if match:
                    try:
                        file_num = int(match.group(1))
                        open_time = int(match.group(2))
                        close_time = int(match.group(3))
                        io_time = int(match.group(4))
                        total_time = int(match.group(5))
                        speed = int(match.group(6))

                        file_numbers.append(file_num)
                        open_times.append(open_time)
                        close_times.append(close_time)
                        io_times.append(io_time)
                        total_times.append(total_time)
                        speeds.append(speed)

                    except ValueError as e:
                        print(f"Warning: Error parsing line {line_num}: {e}")

    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
        return None, None, None, None, None, None
    except Exception as e:
        print(f"Error reading file '{filename}': {e}")
        return None, None, None, None, None, None

    return file_numbers, open_times, close_times, io_times, total_times, speeds

def print_statistics(file_numbers, open_times, close_times, io_times, total_times, speeds):
    """Print statistics"""
    if not file_numbers:
        print("No data to analyze")
        return

    print(f"\n=== Statistics for {len(file_numbers)} files ===")
    print(f"{'Metric':<12} {'Min':<8} {'Max':<8} {'Avg':<8} {'StdDev':<8}")
    print("-" * 50)

    metrics = [
        ('open_time', open_times, 'us'),
        ('close_time', close_times, 'us'),
        ('io_time', io_times, 'us'),
        ('total_time', total_times, 'us'),
        ('speed', speeds, 'KB/s')
    ]

    for name, values, unit in metrics:
        min_val = min(values)
        max_val = max(values)
        avg_val = np.mean(values)
        std_val = np.std(values)
        print(f"{name:<12} {min_val:<8.0f} {max_val:<8.0f} {avg_val:<8.1f} {std_val:<8.1f} {unit}")

def create_graphs(file_numbers, open_times, close_times, io_times, total_times, speeds, log_filename):
    """Create all performance graphs for write operations"""
    if not file_numbers:
        print("No data to plot")
        return

    # Extract filename without path and extension for title
    base_name = os.path.splitext(os.path.basename(log_filename))[0]
    title = f'{base_name} - Performance Analysis'

    # Create comprehensive figure
    fig = plt.figure(figsize=(16, 12))
    fig.suptitle(title, fontsize=18, fontweight='bold')

    # Create subplots in a 3x3 grid
    gs = fig.add_gridspec(3, 3, hspace=0.3, wspace=0.3)

    # 1. Open time
    ax1 = fig.add_subplot(gs[0, 0])
    ax1.plot(file_numbers, open_times, 'b-', linewidth=1, alpha=0.7)
    ax1.scatter(file_numbers, open_times, c='blue', s=6, alpha=0.6)
    ax1.axhline(np.mean(open_times), color='red', linestyle='--', alpha=0.7,
                label=f'Avg: {np.mean(open_times):.0f} μs')
    ax1.set_title('File Open Time')
    ax1.set_ylabel('Time (μs)')
    ax1.grid(True, alpha=0.3)
    ax1.legend(fontsize=8)

    # 2. Close time
    ax2 = fig.add_subplot(gs[0, 1])
    ax2.plot(file_numbers, close_times, 'g-', linewidth=1, alpha=0.7)
    ax2.scatter(file_numbers, close_times, c='green', s=6, alpha=0.6)
    ax2.axhline(np.mean(close_times), color='red', linestyle='--', alpha=0.7,
                label=f'Avg: {np.mean(close_times):.0f} μs')
    ax2.set_title('File Close Time')
    ax2.set_ylabel('Time (μs)')
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=8)

    # 3. I/O time
    ax3 = fig.add_subplot(gs[0, 2])
    ax3.plot(file_numbers, io_times, 'orange', linewidth=1, alpha=0.7)
    ax3.scatter(file_numbers, io_times, c='orange', s=6, alpha=0.6)
    ax3.axhline(np.mean(io_times), color='red', linestyle='--', alpha=0.7,
                label=f'Avg: {np.mean(io_times):.0f} μs')
    ax3.set_title('I/O Time')
    ax3.set_ylabel('Time (μs)')
    ax3.grid(True, alpha=0.3)
    ax3.legend(fontsize=8)

    # 4. Write speed (spanning 2 columns)
    ax4 = fig.add_subplot(gs[1, :2])
    ax4.plot(file_numbers, speeds, 'purple', linewidth=1, alpha=0.7, label='Write Speed')
    ax4.scatter(file_numbers, speeds, c='purple', s=6, alpha=0.6)
    avg_speed = np.mean(speeds)
    min_speed = min(speeds)
    max_speed = max(speeds)
    ax4.axhline(avg_speed, color='red', linestyle='--', alpha=0.7,
               label=f'Avg: {avg_speed:.0f} KB/s')
    ax4.axhline(min_speed, color='orange', linestyle=':', alpha=0.7,
               label=f'Min: {min_speed} KB/s')
    ax4.axhline(max_speed, color='green', linestyle=':', alpha=0.7,
               label=f'Max: {max_speed} KB/s')
    ax4.set_title('Write Speed Over Time')
    ax4.set_xlabel('File Number')
    ax4.set_ylabel('Speed (KB/s)')
    ax4.grid(True, alpha=0.3)
    ax4.legend()

    # 5. Speed histogram
    ax5 = fig.add_subplot(gs[1, 2])
    ax5.hist(speeds, bins=30, alpha=0.7, color='purple', edgecolor='black')
    ax5.axvline(avg_speed, color='red', linestyle='--', alpha=0.7)
    ax5.set_title('Speed Distribution')
    ax5.set_xlabel('Speed (KB/s)')
    ax5.set_ylabel('Count')
    ax5.grid(True, alpha=0.3)
    ax5.text(0.7, 0.9, f'Avg: {avg_speed:.0f}\nStd: {np.std(speeds):.1f}',
             transform=ax5.transAxes, bbox=dict(boxstyle="round", facecolor='white', alpha=0.8),
             fontsize=8)

    # 6. Total time analysis (spanning all columns)
    ax6 = fig.add_subplot(gs[2, :])
    ax6.plot(file_numbers, total_times, 'red', linewidth=1, alpha=0.7, label='Total Time')
    ax6.scatter(file_numbers, total_times, c='red', s=6, alpha=0.6)

    # Add moving average for better trend visualization
    window_size = min(50, len(total_times) // 10)
    if window_size > 1:
        moving_avg = np.convolve(total_times, np.ones(window_size)/window_size, mode='valid')
        ma_x = file_numbers[window_size-1:]
        ax6.plot(ma_x, moving_avg, 'black', linewidth=2, alpha=0.8,
                label=f'Moving Average ({window_size})')

    avg_total = np.mean(total_times)
    ax6.axhline(avg_total, color='blue', linestyle='--', alpha=0.7,
               label=f'Overall Avg: {avg_total:.0f} μs')
    ax6.set_title('Total Operation Time with Trend')
    ax6.set_xlabel('File Number')
    ax6.set_ylabel('Time (μs)')
    ax6.grid(True, alpha=0.3)
    ax6.legend()

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

    print("FS Write Log Parser")
    print("===================")

    # Check if log file exists
    if not os.path.exists(log_file):
        print(f"Error: Log file '{log_file}' does not exist")
        input("Press Enter to exit...")
        return 1

    # Parse log file
    print(f"Parsing log file: {log_file}")
    file_numbers, open_times, close_times, io_times, total_times, speeds = parse_log_file(log_file)

    if not file_numbers:
        print("No valid entries found in log file")
        input("Press Enter to exit...")
        return 1

    print(f"Successfully parsed {len(file_numbers)} entries")

    # Show statistics
    print_statistics(file_numbers, open_times, close_times, io_times, total_times, speeds)

    # Calculate additional insights specific to write operations
    if len(file_numbers) > 10:
        print(f"\n=== Write-Specific Analysis ===")

        # Performance analysis for first file (often has overhead)
        first_file_total = total_times[0]
        avg_total_rest = np.mean(total_times[1:])
        first_file_overhead = ((first_file_total - avg_total_rest) / avg_total_rest) * 100
        print(f"First file overhead: {first_file_overhead:+.1f}% compared to average")

        # Close time analysis (write operations often have variable close times due to flushing)
        close_outliers = [t for t in close_times if t > np.mean(close_times) + 2 * np.std(close_times)]
        if close_outliers:
            print(f"High close times detected: {len(close_outliers)} files (possible flush operations)")
            print(f"Max close time: {max(close_times)} μs vs avg: {np.mean(close_times):.0f} μs")

        # Speed stability analysis
        speed_cv = (np.std(speeds) / np.mean(speeds)) * 100
        print(f"Speed coefficient of variation: {speed_cv:.1f}% ({'stable' if speed_cv < 20 else 'variable'})")

        # Performance degradation analysis
        first_10_avg = np.mean(total_times[:10])
        last_10_avg = np.mean(total_times[-10:])
        degradation = ((last_10_avg - first_10_avg) / first_10_avg) * 100
        print(f"Performance change: {degradation:+.1f}% (first 10 vs last 10 files)")

    # Create graphs
    print("\nGenerating performance graphs...")
    create_graphs(file_numbers, open_times, close_times, io_times, total_times, speeds, log_file)

    print("\nAnalysis complete!")
    input("Press Enter to exit...")
    return 0

if __name__ == "__main__":
    main()
