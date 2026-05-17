import sys
import pandas as pd
import seaborn as sns

import matplotlib

matplotlib.use('Agg')

import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print("Error you need to specify csv file")
    sys.exit(1)

log_filename = sys.argv[1]

try:
    df = pd.read_csv(log_filename, skipinitialspace=True)

    df['Timestamp'] = pd.to_datetime(df['Timestamp'])

    df['GPU_TEMP_C'] = pd.to_numeric(df['GPU_TEMP_C'], errors='coerce')

    sns.set_theme(style="whitegrid")
    plt.figure(figsize=(12, 6))

    sns.lineplot(data=df, x='Timestamp', y='CPU_TEMP_C', label='CPU Temp', color='red', linewidth=2)

    if not df['GPU_TEMP_C'].isna().all():
        sns.lineplot(data=df, x='Timestamp', y='GPU_TEMP_C', label='GPU Temp', color='green', linewidth=2)

    plt.title('Temperature chart', fontsize=16)
    plt.xlabel('Timestamp', fontsize=12)
    plt.ylabel('Temperature[C]', fontsize=12)
    plt.xticks(rotation=45)
    plt.tight_layout()

    output_image = log_filename.replace('.csv', '.png')
    plt.savefig(output_image, dpi=300)
    print(f"Chart saved to {output_image}")
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
