#!/usr/bin/env python3
"""
Convert animation header files to binary format for SPIFFS storage
This script reads .h files with RLE compressed animation data and creates
binary files suitable for on-demand loading from SPIFFS filesystem.
"""

import os
import re
import struct

# Animation files to convert
ANIMATIONS = [
    "angry", "angry_2", "confused_2", "content", "determined",
    "embarrassed", "excited_2", "frustrated", "happy", "happy_2",
    "intro", "laugh", "love", "music", "proud", "relaxed",
    "sleepy", "sleepy_3"
]

def parse_header_file(filename):
    """Parse animation header file and extract metadata and frame data"""

    print(f"\nProcessing {filename}...")

    with open(filename, 'r') as f:
        content = f.read()

    # Extract metadata
    frame_count_match = re.search(r'const int \w+_FRAME_COUNT = (\d+);', content)
    fps_match = re.search(r'const int \w+_FPS = (\d+);', content)
    max_size_match = re.search(r'const int \w+_MAX_COMPRESSED_SIZE = (\d+);', content)

    frame_count = int(frame_count_match.group(1))
    fps = int(fps_match.group(1))
    max_compressed_size = int(max_size_match.group(1))

    print(f"  Frame count: {frame_count}")
    print(f"  FPS: {fps}")
    print(f"  Max compressed size: {max_compressed_size}")

    # Extract frame sizes array
    frame_sizes_match = re.search(r'const int \w+_frame_sizes\[\d+\] = \{([^}]+)\};', content)
    frame_sizes_str = frame_sizes_match.group(1)
    frame_sizes = [int(x.strip()) for x in frame_sizes_str.split(',') if x.strip()]

    # Extract frame data
    frames_match = re.search(r'const uint8_t PROGMEM \w+_frames\[\]\[(\d+)\] = \{(.*?)\n\};', content, re.DOTALL)
    frames_data_str = frames_match.group(2)

    # Parse individual frames
    frame_arrays = re.findall(r'\{([^}]+)\}', frames_data_str)

    frames = []
    for i, frame_str in enumerate(frame_arrays):
        if i >= frame_count:
            break
        # Parse hex values
        hex_values = re.findall(r'0x([0-9A-Fa-f]{2})', frame_str)
        frame_bytes = bytes([int(h, 16) for h in hex_values])
        frames.append(frame_bytes)

    print(f"  Extracted {len(frames)} frames")

    return {
        'frame_count': frame_count,
        'fps': fps,
        'max_compressed_size': max_compressed_size,
        'frame_sizes': frame_sizes,
        'frames': frames
    }

def create_binary_file(anim_name, anim_data, output_dir):
    """Create binary file in SPIFFS format"""

    output_file = os.path.join(output_dir, f"{anim_name}.anim")

    with open(output_file, 'wb') as f:
        # Write header (12 bytes)
        # Format: frame_count (2 bytes), fps (2 bytes), max_compressed_size (2 bytes), reserved (6 bytes)
        f.write(struct.pack('<HHH6x',
                          anim_data['frame_count'],
                          anim_data['fps'],
                          anim_data['max_compressed_size']))

        # Write frame sizes array (2 bytes per frame)
        for size in anim_data['frame_sizes']:
            f.write(struct.pack('<H', size))

        # Write frame data (each frame padded to max_compressed_size)
        for i, frame in enumerate(anim_data['frames']):
            f.write(frame)
            # Pad to max_compressed_size
            padding = anim_data['max_compressed_size'] - len(frame)
            if padding > 0:
                f.write(b'\x00' * padding)

    file_size = os.path.getsize(output_file)
    print(f"  Created {output_file} ({file_size} bytes)")

    return file_size

def create_manifest(animations_info, output_dir):
    """Create manifest file listing all animations"""

    manifest_file = os.path.join(output_dir, "manifest.txt")

    with open(manifest_file, 'w') as f:
        f.write(f"{len(animations_info)}\n")
        for name, info in animations_info.items():
            f.write(f"{name}.anim,{info['frame_count']},{info['fps']},{info['max_compressed_size']}\n")

    print(f"\nCreated manifest: {manifest_file}")

def main():
    """Main conversion process"""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, 'data')

    # Create output directory
    os.makedirs(output_dir, exist_ok=True)

    print("="*60)
    print("Animation Header to SPIFFS Binary Converter")
    print("="*60)

    animations_info = {}
    total_size = 0

    for anim_name in ANIMATIONS:
        header_file = os.path.join(script_dir, f"{anim_name}.h")

        if not os.path.exists(header_file):
            print(f"Warning: {header_file} not found, skipping...")
            continue

        try:
            # Parse header file
            anim_data = parse_header_file(header_file)

            # Create binary file
            file_size = create_binary_file(anim_name, anim_data, output_dir)

            animations_info[anim_name] = anim_data
            total_size += file_size

        except Exception as e:
            print(f"Error processing {anim_name}: {e}")
            import traceback
            traceback.print_exc()

    # Create manifest
    if animations_info:
        create_manifest(animations_info, output_dir)

    print("\n" + "="*60)
    print(f"Conversion complete!")
    print(f"Total animations: {len(animations_info)}")
    print(f"Total size: {total_size:,} bytes ({total_size/1024:.1f} KB)")
    print(f"Output directory: {output_dir}")
    print("="*60)

    print("\nNext steps:")
    print("1. Upload the 'data' folder to ESP32 SPIFFS using Arduino IDE:")
    print("   Tools -> ESP32 Sketch Data Upload")
    print("2. Upload the modified Animation_Test.ino sketch")
    print("3. Run and enjoy all animations without memory issues!")

if __name__ == '__main__':
    main()
