#!/usr/bin/env python3
"""
GIF to Flash Memory Converter
Converts GIF files to C header files with bitmap arrays for ESP32
"""

import cv2
import numpy as np
import os
from pathlib import Path

class GifToFlash:
    def __init__(self, display_width=128, display_height=64):
        self.display_width = display_width
        self.display_height = display_height

    def process_frame(self, frame, flip_horizontal=False):
        """Convert video frame to monochrome bitmap"""
        # Flip horizontally if requested
        if flip_horizontal:
            frame = cv2.flip(frame, 1)

        # Resize to display dimensions
        resized = cv2.resize(frame, (self.display_width, self.display_height))

        # Convert to grayscale
        gray = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)

        # Apply binary threshold
        _, binary = cv2.threshold(gray, 128, 255, cv2.THRESH_BINARY)

        # Convert to bitmap format (1 bit per pixel, packed into bytes)
        bitmap = np.packbits(binary > 0, axis=None)

        return bitmap

    def convert_gif(self, gif_path, output_name, max_frames=60, target_fps=10):
        """
        Convert GIF to C header file

        Args:
            gif_path: Path to GIF file
            output_name: Name for the output (used in variable names)
            max_frames: Maximum number of frames to convert
            target_fps: Target frame rate (will skip frames if needed)
        """
        if not os.path.exists(gif_path):
            print(f"Error: GIF file not found: {gif_path}")
            return False

        print(f"Converting: {gif_path}")
        print(f"Output name: {output_name}")

        # Open GIF
        cap = cv2.VideoCapture(gif_path)
        if not cap.isOpened():
            print(f"Error: Could not open GIF")
            return False

        # Get GIF properties
        original_fps = cap.get(cv2.CAP_PROP_FPS)
        total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

        # Calculate frame skip to achieve target FPS
        frame_skip = max(1, int(original_fps / target_fps))

        print(f"Original: {total_frames} frames at {original_fps} fps")
        print(f"Converting at {target_fps} fps (skip every {frame_skip} frames)")
        print(f"Max frames: {max_frames}")

        # Convert frames
        frames = []
        frame_num = 0
        converted_count = 0

        while converted_count < max_frames:
            ret, frame = cap.read()
            if not ret:
                break

            # Only process every Nth frame
            if frame_num % frame_skip == 0:
                bitmap = self.process_frame(frame)
                frames.append(bitmap)
                converted_count += 1

                if converted_count % 10 == 0:
                    print(f"Processed {converted_count}/{max_frames} frames...")

            frame_num += 1

        cap.release()

        actual_frames = len(frames)
        print(f"Converted {actual_frames} frames")

        # Generate C header file in the same directory as the GIF
        gif_dir = os.path.dirname(gif_path)
        header_path = os.path.join(gif_dir, f"{output_name}.h")
        self.generate_header(frames, output_name, header_path, target_fps)

        return True

    def rle_compress(self, data):
        """
        Run-Length Encoding compression
        Format: [count, value, count, value, ...]
        If count > 255, split into multiple runs
        """
        if len(data) == 0:
            return []

        compressed = []
        current_byte = data[0]
        count = 1

        for i in range(1, len(data)):
            if data[i] == current_byte and count < 255:
                count += 1
            else:
                compressed.append(count)
                compressed.append(current_byte)
                current_byte = data[i]
                count = 1

        # Add the last run
        compressed.append(count)
        compressed.append(current_byte)

        return np.array(compressed, dtype=np.uint8)

    def generate_header(self, frames, name, output_path, fps, use_compression=True):
        """Generate C header file with frame data"""

        frame_count = len(frames)
        bytes_per_frame = len(frames[0]) if frames else 0

        # Compress frames if enabled
        if use_compression:
            compressed_frames = [self.rle_compress(frame) for frame in frames]
            compressed_sizes = [len(cf) for cf in compressed_frames]
            max_compressed_size = max(compressed_sizes) if compressed_sizes else 0
            total_compressed = sum(compressed_sizes)
            total_uncompressed = frame_count * bytes_per_frame
            compression_ratio = (1 - total_compressed / total_uncompressed) * 100 if total_uncompressed > 0 else 0
        else:
            compressed_frames = frames
            max_compressed_size = bytes_per_frame
            total_compressed = frame_count * bytes_per_frame
            compression_ratio = 0

        with open(output_path, 'w') as f:
            # Header guard
            guard_name = f"{name.upper()}_H"
            f.write(f"#ifndef {guard_name}\n")
            f.write(f"#define {guard_name}\n\n")

            f.write(f"// Auto-generated GIF data for {name}\n")
            f.write(f"// Frame count: {frame_count}\n")
            f.write(f"// Frame rate: {fps} FPS\n")
            f.write(f"// Uncompressed bytes per frame: {bytes_per_frame}\n")

            if use_compression:
                f.write(f"// Compression: RLE (Run-Length Encoding)\n")
                f.write(f"// Compressed size: {total_compressed} bytes\n")
                f.write(f"// Uncompressed size: {total_uncompressed} bytes\n")
                f.write(f"// Compression ratio: {compression_ratio:.1f}% saved\n")
            else:
                f.write(f"// Total size: {total_compressed} bytes\n")

            f.write("\n#include <Arduino.h>\n\n")

            # Frame count and FPS constants
            f.write(f"const int {name}_FRAME_COUNT = {frame_count};\n")
            f.write(f"const int {name}_FPS = {fps};\n")
            f.write(f"const int {name}_FRAME_DELAY = {int(1000/fps)}; // milliseconds\n")
            f.write(f"const int {name}_UNCOMPRESSED_SIZE = {bytes_per_frame};\n")

            if use_compression:
                f.write(f"const int {name}_MAX_COMPRESSED_SIZE = {max_compressed_size};\n\n")

                # Add RLE decompression function
                f.write(f"// RLE Decompression function\n")
                f.write(f"// Decompresses RLE data into output buffer\n")
                f.write(f"// compressed: pointer to compressed data\n")
                f.write(f"// compressed_size: size of compressed data\n")
                f.write(f"// output: buffer to store decompressed data (must be {bytes_per_frame} bytes)\n")
                f.write(f"void {name}_decompress(const uint8_t* compressed, int compressed_size, uint8_t* output) {{\n")
                f.write(f"  int out_idx = 0;\n")
                f.write(f"  for (int i = 0; i < compressed_size; i += 2) {{\n")
                f.write(f"    uint8_t count = compressed[i];\n")
                f.write(f"    uint8_t value = compressed[i + 1];\n")
                f.write(f"    for (uint8_t j = 0; j < count; j++) {{\n")
                f.write(f"      output[out_idx++] = value;\n")
                f.write(f"    }}\n")
                f.write(f"  }}\n")
                f.write(f"}}\n\n")

                # Compressed frame sizes array
                f.write(f"const int {name}_frame_sizes[{frame_count}] = {{\n")
                f.write("  ")
                for i, size in enumerate(compressed_sizes):
                    f.write(f"{size}")
                    if i < frame_count - 1:
                        f.write(", ")
                    if (i + 1) % 10 == 0 and i < frame_count - 1:
                        f.write("\n  ")
                f.write("\n};\n\n")
            else:
                f.write("\n")

            # Frame data array
            if use_compression:
                f.write(f"const uint8_t PROGMEM {name}_frames[][{max_compressed_size}] = {{\n")
            else:
                f.write(f"const uint8_t PROGMEM {name}_frames[][{bytes_per_frame}] = {{\n")

            for i, frame_data in enumerate(compressed_frames):
                f.write("  {")
                for j, byte in enumerate(frame_data):
                    if j > 0:
                        f.write(",")
                    if j % 16 == 0:
                        f.write("\n    ")
                    f.write(f"0x{byte:02X}")

                # Pad with zeros if using compression and frame is smaller than max
                if use_compression and len(frame_data) < max_compressed_size:
                    for j in range(len(frame_data), max_compressed_size):
                        f.write(",")
                        if j % 16 == 0:
                            f.write("\n    ")
                        f.write("0x00")

                f.write("\n  }")
                if i < frame_count - 1:
                    f.write(",")
                f.write(f"  // Frame {i}")
                if use_compression:
                    f.write(f" - {compressed_sizes[i]} bytes")
                f.write("\n")

            f.write("};\n\n")

            f.write(f"#endif // {guard_name}\n")

        print(f"Generated: {output_path}")
        if use_compression:
            print(f"Uncompressed size: {total_uncompressed / 1024:.2f} KB")
            print(f"Compressed size: {total_compressed / 1024:.2f} KB")
            print(f"Space saved: {compression_ratio:.1f}%")
        else:
            print(f"Size: {total_compressed / 1024:.2f} KB")


def main():
    converter = GifToFlash(128, 64)

    # Get the directory where this script is located
    script_dir = Path(__file__).parent

    # Find all .gif files in the current directory
    gif_files = sorted(script_dir.glob("*.gif"))

    if not gif_files:
        print("No .gif files found in the current directory")
        return

    print("="*60)
    print("GIF to Flash Memory Converter")
    print("="*60)
    print(f"Found {len(gif_files)} GIF files")
    print()

    # Process each GIF file
    for gif_path in gif_files:
        # Create output name from filename (remove .gif extension and replace spaces/special chars)
        output_name = gif_path.stem.replace(" ", "_").replace("-", "_").lower()

        # Convert GIF (max 60 frames, 10 fps)
        converter.convert_gif(str(gif_path), output_name, max_frames=60, target_fps=10)
        print()

    print("="*60)
    print("Conversion complete!")
    print("="*60)
    print("\nGenerated header files:")
    for gif_path in gif_files:
        output_name = gif_path.stem.replace(" ", "_").replace("-", "_").lower()
        header_file = f"{output_name}.h"
        header_path = script_dir / header_file
        if header_path.exists():
            size_kb = header_path.stat().st_size / 1024
            print(f"  - {header_file} ({size_kb:.2f} KB)")


if __name__ == "__main__":
    main()
