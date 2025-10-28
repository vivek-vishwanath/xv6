import numpy as np
import matplotlib.pyplot as plt
import sys
import re
import os

def extract_response_time(line):
    response_pattern = re.compile(r'response (\d+)')
    match = response_pattern.search(line)
    if match:
        return int(match.group(1))
    return None


def read_log_file(filename):
    response_times = []

    with open(filename, 'r') as file:
        for line in file:
            response_time = extract_response_time(line)
            if response_time is not None:
                response_times.append(response_time)

    return response_times


def plot_data(file, label):
    data = read_log_file(file)

    # Sort data in ascending order
    sorted_data = np.sort(data)

    # Calculate the CDF values
    cdf = np.arange(1, len(sorted_data) + 1) / len(sorted_data)

    # Plot the CDF
    plt.plot(sorted_data, cdf, linestyle='-', label=label)

if len(sys.argv) > 1:
    for file in os.listdir(sys.argv[1]):
        plot_data(f"{sys.argv[1]}/{file}", label=os.path.splitext(file)[0])

cpus = ""

if len(sys.argv) > 2:
    cpus = sys.argv[2]

title = f"Cumulative Distribution Function with {cpus} CPU"

plural = cpus != "1"
if plural:
    title += "s"

plt.xlabel('Response Latency (Ticks)')
plt.ylabel('Latency CDF')
plt.title(title)
plt.legend()
plt.grid(True)
plt.show()
