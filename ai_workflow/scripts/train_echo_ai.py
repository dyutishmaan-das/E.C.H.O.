import os
import numpy as np
import tensorflow as tf
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle

# 1. SETUP ORGANIZED PATHS
ROOT_DIR = r"C:\Users\dyuti\Projects\Project-ECHO"
DATASET_PATH = os.path.join(ROOT_DIR, "radar_dataset", "Zendo_dataset", "data_SAAB_SIRS_77GHz_FMCW.npy")
EXPORT_DIR = os.path.join(ROOT_DIR, "ai_workflow", "exported_models")

if not os.path.exists(EXPORT_DIR):
    os.makedirs(EXPORT_DIR)

print(f"Loading massive dataset from: {DATASET_PATH}")
data = np.load(DATASET_PATH, allow_pickle=True)

# 2. SEPARATE DRONES VS BIRDS VS HUMANS
drones = ['D1', 'D2', 'D3', 'D4', 'D5', 'D6']
birds = ['seagull', 'pigeon', 'raven', 'black-headed gull', 'heron', 'seagull and black-headed gull']
humans = ['human_walk', 'human_run']

X_list = []
y_list = []

print("Extracting classes from Complex radar numbers...")
for row in data:
    label_str = row[0]
    
    # Is it a Bird (0), Drone (1), or Human (2)?
    if label_str in birds:
        label = 0
    elif label_str in drones:
        label = 1
    elif label_str in humans:
        label = 2
    else:
        continue # Ignore 'CR' (corner reflectors) and noise
    
    # Extract the 1280 complex numbers and convert them to Real Magnitudes
    # The readMe says Column 2 is actually a matrix of size (1280 rows, N columns)
    complex_data = row[1] 
    real_magnitude_matrix = np.abs(complex_data) 
    
    # We must slice out each of the N segments (columns) individually!
    num_segments = real_magnitude_matrix.shape[1]
    for i in range(num_segments):
        single_segment = real_magnitude_matrix[:, i]
        X_list.append(single_segment)
        y_list.append(label)

X = np.array(X_list)
y = np.array(y_list)

# Shuffle the data
X, y = shuffle(X, y, random_state=42)

# 3. NORMALIZE THE DATA FOR THE ESP8266
# Instead of complex Z-scores, we just divide by the maximum possible value 
# so the ESP8266 only has to do one division operation!
global_max_val = np.max(X)
X = X / global_max_val

# Reshape for Convolutional 1D network
X = X.reshape((X.shape[0], X.shape[1], 1))

# Split into Training and Testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

print(f"\nExtracted {len(X_train)} training samples and {len(X_test)} testing samples.")
print(f"Data Shape: {X_train.shape}")
print(f"IMPORTANT: Store this Global Max Value in your ESP8266 code -> {global_max_val:.4f}\n")

# THE SCIENTIFIC LIMIT PUSH: We are exponentially increasing the width of the 2-layer system
# and shrinking the Strides from 4 to 2 so it physically cannot ignore the micro-Doppler details.
# This raises the accuracy ceiling immensely while preserving the beautifully smooth curve!
model = tf.keras.models.Sequential([
    # Layer 1: Massive 64-Filter High-Resolution Sweep
    tf.keras.layers.Conv1D(filters=64, kernel_size=16, strides=2, padding='same', activation='relu', input_shape=(1280, 1)),
    tf.keras.layers.BatchNormalization(),
    tf.keras.layers.MaxPooling1D(pool_size=4),
    
    # Layer 2: Massive 128-Filter Sub-Pattern Analysis
    tf.keras.layers.Conv1D(filters=128, kernel_size=8, strides=2, padding='same', activation='relu'),
    tf.keras.layers.BatchNormalization(),
    tf.keras.layers.GlobalAveragePooling1D(),
    
    # Final Decision Matrix
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.BatchNormalization(),
    tf.keras.layers.Dropout(0.3),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dropout(0.2),
    tf.keras.layers.Dense(3, activation='softmax') # 3 Outputs: Bird (0), Drone (1), Human (2)
])

# Use a slightly higher initial learning rate with ReduceLROnPlateau for optimal convergence
adam_optimizer = tf.keras.optimizers.Adam(learning_rate=0.001)
model.compile(optimizer=adam_optimizer, loss='sparse_categorical_crossentropy', metrics=['accuracy'])

# Print architecture size
model.summary()

# 5. TRAIN THE MODEL LOCALLY (No 100MB Cloud Limits!)
print("\n--- INITIATING LOCAL TRAINING ---")
# EARLY STOPPING TEACHER: 
# "Stop studying if accuracy drops for 10 straight exams, and save the smartest brain!"
early_stop = tf.keras.callbacks.EarlyStopping(monitor='val_accuracy', patience=12, restore_best_weights=True)
lr_scheduler = tf.keras.callbacks.ReduceLROnPlateau(monitor='val_loss', factor=0.5, patience=4, min_lr=1e-6, verbose=1)

history = model.fit(X_train, y_train, epochs=100, batch_size=128, validation_data=(X_test, y_test), callbacks=[early_stop, lr_scheduler])

# --- GENERATE ACCURACY GRAPH ---
print("\nGenerating Training Accuracy & Loss Graphs...")
plt.figure(figsize=(12, 5))
plt.subplot(1, 2, 1)
plt.plot(history.history['accuracy'], label='Training Accuracy', color='green', linewidth=2)
plt.plot(history.history['val_accuracy'], label='Validation Accuracy', color='blue', linestyle='dashed')
plt.title('E.C.H.O Model Accuracy (Birds vs Drones vs Humans)')
plt.ylabel('Accuracy')
plt.xlabel('Epoch')
plt.legend()

plt.subplot(1, 2, 2)
plt.plot(history.history['loss'], label='Training Loss', color='red', linewidth=2)
plt.plot(history.history['val_loss'], label='Validation Loss', color='orange', linestyle='dashed')
plt.title('E.C.H.O Model Loss')
plt.ylabel('Loss')
plt.xlabel('Epoch')
plt.legend()

graph_path = os.path.join(EXPORT_DIR, "training_history.png")
plt.savefig(graph_path)
print(f"Graph successfully saved to: {graph_path}")
plt.close()

# 6. EXPORT TO ESP8266 TENSORFLOW LITE (.tflite) FORMAT
print("\nCompressing model into TFLite format for Microcontrollers...")
converter = tf.lite.TFLiteConverter.from_keras_model(model)
# Optimize the weights to 8-bit integers to save even more RAM!
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

tflite_path = os.path.join(EXPORT_DIR, "echo_model.tflite")
with open(tflite_path, "wb") as f:
    f.write(tflite_model)
print(f"Saved binary TFLite file -> {tflite_path}")

# 7. CONVERT TO C++ HEADER FILE (.h)
def convert_to_c_array(tflite_model_byte_array):
    hex_array = [hex(b) for b in tflite_model_byte_array]
    c_str = ""
    for i, h in enumerate(hex_array):
        c_str += f"{h}, "
        if (i + 1) % 12 == 0:
            c_str += "\n  "
    return c_str

cpp_path = os.path.join(EXPORT_DIR, "echo_model.h")
with open(cpp_path, "w") as f:
    f.write(f"// E.C.H.O. Automated Target Recognition Pipeline\n")
    f.write(f"// Global Normalization Divisor: {global_max_val:.4f}\n\n")
    f.write("#ifndef ECHO_MODEL_H\n")
    f.write("#define ECHO_MODEL_H\n\n")
    f.write("alignas(8) const unsigned char echo_model_tflite[] = {\n  ")
    f.write(convert_to_c_array(tflite_model))
    f.write("\n};\n")
    f.write(f"const unsigned int echo_model_tflite_len = {len(tflite_model)};\n\n")
    f.write("#endif // ECHO_MODEL_H\n")

print(f"\nSUCCESS! C++ Header successfully created for your ESP8266 -> {cpp_path}")
print("You can now `#include \"echo_model.h\"` inside the Arduino IDE!")
