import psutil
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Step 1: Define a function to retrieve live process information
def get_process_info():
    processes = []
    for process in psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_percent']):
        try:
            processes.append(process.info)
        except psutil.NoSuchProcess:
            pass  # Process might terminate, so we skip those
    return processes

# Step 2: Define the update function that will refresh the data and the plot
def update(frame):
    # Clear the current plot
    ax[0].cla()
    ax[1].cla()

    # Retrieve the latest process information
    process_info = get_process_info()
    df = pd.DataFrame(process_info)
    
    # Sort by CPU usage and take the top 10 processes
    df = df.sort_values(by='cpu_percent', ascending=False).head(10)

    # Plot CPU usage
    ax[0].barh(df['name'], df['cpu_percent'], color='skyblue')
    ax[0].set_xlabel('CPU Usage (%)')
    ax[0].set_title('Top 10 Processes by CPU Usage')

    # Plot Memory usage
    ax[1].barh(df['name'], df['memory_percent'], color='lightgreen')
    ax[1].set_xlabel('Memory Usage (%)')
    ax[1].set_title('Top 10 Processes by Memory Usage')

    # Tighten layout
    plt.tight_layout()

# Step 3: Create the figure and axes
fig, ax = plt.subplots(1, 2, figsize=(12, 6))

# Step 4: Create the animation
ani = FuncAnimation(fig, update, interval=500)  # Updates every 1000 ms (1 second)

# Show the live plot
plt.show()
