import os
import csv
import math

# File paths
input_file = r"C:\Users\dyuti\Projects\Project-ECHO\radar_dataset\edge_impulse_csv\train.csv"
output_dir = r"C:\Users\dyuti\Projects\Project-ECHO\radar_dataset\split_train_csvs"

# Instead of memory-crashing on 1GB, we read chunk by chunk
chunk_size = 5000  # Split every 5,000 samples. This makes files tiny!

def split_large_csv():
    if not os.path.exists(input_file):
        print(f"Error: Could not find {input_file}")
        return

    # Create destination folder if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"Created folder: {output_dir}")

    print(f"Splitting {input_file} into pieces of {chunk_size} lines...")

    with open(input_file, 'r', encoding='utf-8') as infile:
        # Grab the column headers (LABEL, F0, F1...) so every chunk has them
        header = infile.readline()

        chunk_id = 1
        current_line_count = 0
        current_out_file = None

        for line in infile:
            # If start of a new chunk, open a new file
            if current_line_count == 0:
                output_filename = os.path.join(output_dir, f"train_part{chunk_id:03d}.csv")
                current_out_file = open(output_filename, 'w', encoding='utf-8')
                current_out_file.write(header)
                print(f"Writing {output_filename}...")
            
            # Write data row
            current_out_file.write(line)
            current_line_count += 1

            # If chunk is full, close it and increment logic
            if current_line_count == chunk_size:
                current_out_file.close()
                current_line_count = 0
                chunk_id += 1

        # Clean up last file if not fully closed
        if current_out_file and not current_out_file.closed:
            current_out_file.close()

    print(f"\nSuccess! Successfully split your massive CSV into {chunk_id} smaller, bite-sized files.")
    print(f"You can find them here: {output_dir}")

if __name__ == "__main__":
    split_large_csv()
