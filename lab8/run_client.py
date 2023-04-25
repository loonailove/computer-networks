import subprocess
import re
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import matplotlib.ticker as ticker

def get_cwnd_throughput(port):
    result = subprocess.run(["ss", "-tin", f'sport = :{port} or dport = :{port}'], stdout=subprocess.PIPE)
    output = result.stdout.decode("utf-8")
    cwnd_regex = re.compile(r"cwnd:(\d+)")
    cwnd = cwnd_regex.findall(output)
    
    throughput_regex = re.compile(r"send (\d+\.?\d+?)M?bps")
    throughput = throughput_regex.findall(output)

    return (int(cwnd[0]), float(throughput[0]) / 10**6) if cwnd and throughput else (None, None)

def run_iperf_client(target_ip, port):
    # TODO: Here we change the parameters to the iperf command
    return subprocess.Popen(["iperf", "-c", target_ip, "-p", str(port), "-t", "0"])

def update_plot(frame, cwnd_values, throughput_values):
    # Only keep the last 120 points (max)
    del cwnd_values[:-120]
    del throughput_values[:-120]
    
    cwnd, throughput = get_cwnd_throughput(port)
    if cwnd is not None and throughput is not None:
        cwnd_values.append(cwnd)
        throughput_values.append(throughput)

        ax1.clear()
        ax1.plot(cwnd_values)
        ax1.set_xlabel('Time (0.25s intervals)')
        ax1.set_ylabel('Congestion Window Size (cwnd)')
        ax1.set_title('Congestion Window Size vs. Time')
        ax1.grid(visible=True, axis='y', linestyle=':')
        ax1.set_ylim(bottom=0)

        ax2.clear()
        ax2.plot(throughput_values)
        ax2.set_xlabel('Time (0.25s intervals)')
        ax2.set_ylabel('Throughput (Mbps)')
        ax2.set_title('Throughput vs. Time')
        ax2.set_ylim(bottom=0)
        ax2.grid(visible=True, axis='y', linestyle=':')
        ax2.yaxis.set_major_locator(ticker.MultipleLocator(2))
        ax2.yaxis.set_minor_locator(ticker.MultipleLocator(1))

if __name__ == "__main__":
    target_ip = "172.16.0.100"
    port = 5001

    cwnd_values = []
    throughput_values = []

    try:
        iperf_client = run_iperf_client(target_ip, port)

        #fig, ax = plt.subplots()
        fig, (ax1, ax2) = plt.subplots(nrows=2, ncols=1, sharex=True)
        fig.tight_layout(pad=3)
        ani = FuncAnimation(fig, update_plot, fargs=(cwnd_values, throughput_values,), interval=250, blit=False)
        plt.show()
        iperf_client.terminate()
    except:
        print("\nTerminating iperf client...")
        iperf_client.terminate()
