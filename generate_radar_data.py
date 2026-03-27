import os
import math
import random
import csv

# Configuration
SAMPLE_RATE_HZ = 1000
DURATION_SECONDS = 2
SAMPLES_PER_FILE = SAMPLE_RATE_HZ * DURATION_SECONDS
NUM_FILES_PER_CLASS = 20
OUTPUT_DIR = "radar_dataset"

os.makedirs(OUTPUT_DIR, exist_ok=True)

def generate_empty_air():
    data = []
    for i in range(SAMPLES_PER_FILE):
        noise = random.uniform(-50, 50)
        val = int(2048 + noise)
        data.append((i, val)) # timestamp is i milliseconds (since sample rate is 1000Hz)
    return data

def generate_bird():
    data = []
    # Birds flap wings slowly, around 3-8 Hz.
    flap_rate = random.uniform(3, 8)
    for i in range(SAMPLES_PER_FILE):
        t = i / SAMPLE_RATE_HZ
        # Main body reflection (slow drift) + wing flap reflection
        body = math.sin(2 * math.pi * 0.5 * t) * 100 
        wings = math.sin(2 * math.pi * flap_rate * t) * 400
        noise = random.uniform(-100, 100)
        val = int(2048 + body + wings + noise)
        val = max(0, min(4095, val)) # Clamp to STM32 12-bit ADC range
        data.append((i, val))
    return data

def generate_drone():
    data = []
    # Drones have 4 high-RPM propellers. 6000 RPM -> ~100 Hz.
    # Four motors spinning slightly out of sync create a complex, chaotic high-frequency wave.
    f1 = random.uniform(90, 110)
    f2 = random.uniform(90, 110)
    f3 = random.uniform(90, 110)
    f4 = random.uniform(90, 110)
    for i in range(SAMPLES_PER_FILE):
        t = i / SAMPLE_RATE_HZ
        prop1 = math.sin(2 * math.pi * f1 * t) * 150
        prop2 = math.sin(2 * math.pi * f2 * t) * 150
        prop3 = math.sin(2 * math.pi * f3 * t) * 150
        prop4 = math.sin(2 * math.pi * f4 * t) * 150
        noise = random.uniform(-200, 200) # Drones reflect more noisy scatter
        val = int(2048 + prop1 + prop2 + prop3 + prop4 + noise)
        val = max(0, min(4095, val))
        data.append((i, val))
    return data

def save_csv(filename, data):
    filepath = os.path.join(OUTPUT_DIR, filename)
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['timestamp', 'radar_adc'])
        writer.writerows(data)

print(f"Generating Simulated Radar Dataset in {OUTPUT_DIR}/ ...")

for i in range(NUM_FILES_PER_CLASS):
    # Naming format: <label>.<file_number>.csv
    # Edge Impulse automatically reads the label from the text before the first period!
    save_csv(f"empty.{i:02d}.csv", generate_empty_air())
    save_csv(f"bird.{i:02d}.csv", generate_bird())
    save_csv(f"drone.{i:02d}.csv", generate_drone())

print(f"Dataset generation complete! {NUM_FILES_PER_CLASS * 3} CSV files created.")
