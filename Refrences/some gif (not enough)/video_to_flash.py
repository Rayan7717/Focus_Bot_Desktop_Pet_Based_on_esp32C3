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

    def generate_header(self, frames, name, output_path, fps):
        """Generate C header file with frame data"""

        frame_count = len(frames)
        bytes_per_frame = len(frames[0]) if frames else 0

        with open(output_path, 'w') as f:
            # Header guard
            guard_name = f"{name.upper()}_H"
            f.write(f"#ifndef {guard_name}\n")
            f.write(f"#define {guard_name}\n\n")

            f.write(f"// Auto-generated GIF data for {name}\n")
            f.write(f"// Frame count: {frame_count}\n")
            f.write(f"// Frame rate: {fps} FPS\n")
            f.write(f"// Bytes per frame: {bytes_per_frame}\n")
            f.write(f"// Total size: {frame_count * bytes_per_frame} bytes\n\n")

            f.write("#include <Arduino.h>\n\n")

            # Frame count and FPS constants
            f.write(f"const int {name}_FRAME_COUNT = {frame_count};\n")
            f.write(f"const int {name}_FPS = {fps};\n")
            f.write(f"const int {name}_FRAME_DELAY = {int(1000/fps)}; // milliseconds\n\n")

            # Frame data array
            f.write(f"const uint8_t PROGMEM {name}_frames[][{bytes_per_frame}] = {{\n")

            for i, frame in enumerate(frames):
                f.write("  {")
                for j, byte in enumerate(frame):
                    if j > 0:
                        f.write(",")
                    if j % 16 == 0:
                        f.write("\n    ")
                    f.write(f"0x{byte:02X}")
                f.write("\n  }")
                if i < frame_count - 1:
                    f.write(",")
                f.write(f"  // Frame {i}\n")

            f.write("};\n\n")

            f.write(f"#endif // {guard_name}\n")

        print(f"Generated: {output_path}")
        print(f"Size: {frame_count * bytes_per_frame / 1024:.2f} KB")


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
