import os
import re
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
from pathlib import Path
from collections import defaultdict
import tkinter as tk
from tkinter import filedialog, messagebox

class FrameAnalyzer:
    def __init__(self, frames_directory, byte_format='vertical'):
        self.frames_dir = Path(frames_directory)
        self.frames = {}
        self.width = 128
        self.height = 64
        self.byte_format = byte_format
        
    def parse_frame_file(self, filepath):
        """Parse a .h file and extract bitmap data"""
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Extract frame number from filename - handles both frame_1.h and frame_00001.h
        match = re.search(r'frame[_-]0*(\d+)', filepath.name)
        if not match:
            return None
        frame_num = int(match.group(1))
        
        # Extract hex data
        hex_data = re.findall(r'0x[0-9A-Fa-f]{2}', content)
        
        if not hex_data:
            return None
            
        # Convert hex strings to integers
        byte_array = [int(x, 16) for x in hex_data]
        
        return frame_num, byte_array
    
    def bytes_to_image(self, byte_array):
        """Convert byte array to PIL Image (128x64 monochrome)"""
        img_array = np.zeros((self.height, self.width), dtype=np.uint8)
        
        # OLED format: 128 columns × 8 pages (8 rows of 8 pixels each)
        # Each byte represents 8 vertical pixels in a column
        # Layout: bytes are arranged as columns (x), then pages (y/8)
        
        byte_idx = 0
        for page in range(self.height // 8):  # 8 pages for 64 pixel height
            for x in range(self.width):  # 128 columns
                if byte_idx < len(byte_array):
                    byte_val = byte_array[byte_idx]
                    # Each bit in the byte represents a pixel
                    # LSB = top pixel of the page
                    for bit in range(8):
                        y = page * 8 + bit
                        if byte_val & (1 << bit):
                            img_array[y, x] = 255
                    byte_idx += 1
        
        return Image.fromarray(img_array, mode='L')
    
    def bytes_to_image_horizontal(self, byte_array):
        """Alternative: Convert byte array with horizontal byte ordering"""
        img_array = np.zeros((self.height, self.width), dtype=np.uint8)
        
        # Horizontal format: each byte represents 8 horizontal pixels
        # Layout: left to right, top to bottom
        
        byte_idx = 0
        for y in range(self.height):
            for x_byte in range(self.width // 8):  # 16 bytes per row
                if byte_idx < len(byte_array):
                    byte_val = byte_array[byte_idx]
                    for bit in range(8):
                        x = x_byte * 8 + bit
                        # MSB is leftmost pixel
                        if byte_val & (1 << (7 - bit)):
                            img_array[y, x] = 255
                    byte_idx += 1
        
        return Image.fromarray(img_array, mode='L')
    
    def test_both_formats(self, frame_num):
        """Test both byte ordering formats for a specific frame"""
        if frame_num not in self.frames:
            print(f"Frame {frame_num} not found")
            return
        
        byte_array = self.frames[frame_num]['data']
        
        # Generate both versions
        img_vertical = self.bytes_to_image(byte_array)
        img_horizontal = self.bytes_to_image_horizontal(byte_array)
        
        # Display both
        fig, axes = plt.subplots(1, 2, figsize=(12, 5))
        fig.suptitle(f'Frame {frame_num} - Format Comparison', fontsize=14)
        
        axes[0].imshow(img_vertical, cmap='gray')
        axes[0].set_title('Vertical Page Format (Current)')
        axes[0].axis('off')
        
        axes[1].imshow(img_horizontal, cmap='gray')
        axes[1].set_title('Horizontal Format')
        axes[1].axis('off')
        
        plt.tight_layout()
        plt.show()
        
        return img_vertical, img_horizontal
    
    def load_all_frames(self):
        """Load all frame files from directory"""
        # Try multiple patterns to find frame files
        frame_files = list(self.frames_dir.glob('frame_*.h'))
        if not frame_files:
            frame_files = list(self.frames_dir.glob('frame*.h'))
        
        # Sort by frame number, not filename
        def get_frame_num(filepath):
            match = re.search(r'frame[_-]?0*(\d+)', filepath.name)
            return int(match.group(1)) if match else 0
        
        frame_files.sort(key=get_frame_num)
        
        print(f"Found {len(frame_files)} frame files")
        print("Loading frames", end='', flush=True)
        
        loaded_count = 0
        for i, filepath in enumerate(frame_files):
            result = self.parse_frame_file(filepath)
            if result:
                frame_num, byte_array = result
                # Use the selected byte format
                if self.byte_format == 'horizontal':
                    img = self.bytes_to_image_horizontal(byte_array)
                else:
                    img = self.bytes_to_image(byte_array)
                    
                self.frames[frame_num] = {
                    'image': img,
                    'data': byte_array,
                    'path': filepath
                }
                loaded_count += 1
            
            # Progress indicator
            if (i + 1) % 50 == 0:
                print('.', end='', flush=True)
        
        print()  # New line
        print(f"Successfully loaded {len(self.frames)} frames")
        return self.frames
    
    def calculate_frame_difference(self, frame1_data, frame2_data):
        """Calculate difference between two frames"""
        if len(frame1_data) != len(frame2_data):
            return float('inf')
        
        diff = sum(abs(a - b) for a, b in zip(frame1_data, frame2_data))
        return diff / len(frame1_data)
    
    def group_frames_by_similarity(self, threshold=10.0):
        """Group frames into videos based on scene changes"""
        if not self.frames:
            self.load_all_frames()
        
        frame_numbers = sorted(self.frames.keys())
        groups = []
        current_group = [frame_numbers[0]]
        
        for i in range(1, len(frame_numbers)):
            prev_frame = frame_numbers[i-1]
            curr_frame = frame_numbers[i]
            
            # Calculate difference
            diff = self.calculate_frame_difference(
                self.frames[prev_frame]['data'],
                self.frames[curr_frame]['data']
            )
            
            # If difference is large, start a new group
            if diff > threshold:
                groups.append(current_group)
                current_group = [curr_frame]
            else:
                current_group.append(curr_frame)
        
        # Add last group
        if current_group:
            groups.append(current_group)
        
        return groups
    
    def preview_frames(self, frame_numbers=None, rows=None, cols=None, title='Frame Preview'):
        """Preview selected frames in a grid"""
        if not self.frames:
            self.load_all_frames()
        
        if frame_numbers is None:
            # Show all frames
            frame_numbers = sorted(self.frames.keys())
        
        # Auto-calculate grid size if not provided
        if rows is None or cols is None:
            total_frames = len(frame_numbers)
            if total_frames <= 24:
                cols = 6
                rows = (total_frames + cols - 1) // cols
            elif total_frames <= 100:
                cols = 10
                rows = (total_frames + cols - 1) // cols
            else:
                # For large numbers, calculate square-ish grid
                cols = int(np.ceil(np.sqrt(total_frames)))
                rows = (total_frames + cols - 1) // cols
        
        # Adjust figure size based on number of frames
        fig_width = min(20, cols * 2)
        fig_height = min(30, rows * 1.5)
        
        fig, axes = plt.subplots(rows, cols, figsize=(fig_width, fig_height))
        fig.suptitle(title, fontsize=16)
        
        # Handle single subplot case
        if rows == 1 and cols == 1:
            axes = np.array([[axes]])
        elif rows == 1 or cols == 1:
            axes = axes.reshape(rows, cols)
        
        axes = axes.flatten()
        
        for idx, frame_num in enumerate(frame_numbers):
            if idx >= len(axes):
                break
            
            if frame_num in self.frames:
                axes[idx].imshow(self.frames[frame_num]['image'], cmap='gray')
                axes[idx].set_title(f'{frame_num}', fontsize=8)
                axes[idx].axis('off')
        
        # Hide unused subplots
        for idx in range(len(frame_numbers), len(axes)):
            axes[idx].axis('off')
        
        plt.tight_layout()
        plt.show()
    
    def preview_all_frames_paginated(self, frames_per_page=50):
        """Preview all frames in paginated views"""
        if not self.frames:
            self.load_all_frames()
        
        all_frames = sorted(self.frames.keys())
        total_pages = (len(all_frames) + frames_per_page - 1) // frames_per_page
        
        print(f"\nTotal frames: {len(all_frames)}")
        print(f"Showing {frames_per_page} frames per page")
        print(f"Total pages: {total_pages}")
        
        for page in range(total_pages):
            start_idx = page * frames_per_page
            end_idx = min(start_idx + frames_per_page, len(all_frames))
            page_frames = all_frames[start_idx:end_idx]
            
            print(f"\nShowing page {page + 1}/{total_pages} (Frames {start_idx} to {end_idx - 1})")
            self.preview_frames(page_frames, title=f'All Frames - Page {page + 1}/{total_pages}')
            
            if page < total_pages - 1:
                input("Press Enter to view next page...")  # Pause between pages
    
    def preview_groups(self, groups, frames_per_group=4):
        """Preview first few frames from each group"""
        num_groups = len(groups)
        fig, axes = plt.subplots(num_groups, frames_per_group, 
                                 figsize=(15, 3 * num_groups))
        
        if num_groups == 1:
            axes = [axes]
        
        fig.suptitle(f'Video Groups Preview ({num_groups} videos detected)', 
                     fontsize=16)
        
        for group_idx, group in enumerate(groups):
            preview_frames = group[:frames_per_group]
            
            for frame_idx, frame_num in enumerate(preview_frames):
                if frame_num in self.frames:
                    axes[group_idx][frame_idx].imshow(
                        self.frames[frame_num]['image'], cmap='gray'
                    )
                    axes[group_idx][frame_idx].set_title(f'F{frame_num}')
                    axes[group_idx][frame_idx].axis('off')
            
            # Add group label
            axes[group_idx][0].text(-0.1, 0.5, f'Video {group_idx + 1}\n({len(group)} frames)', 
                                   transform=axes[group_idx][0].transAxes,
                                   fontsize=10, verticalalignment='center',
                                   bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
            
            # Hide unused subplots in this row
            for frame_idx in range(len(preview_frames), frames_per_group):
                axes[group_idx][frame_idx].axis('off')
        
        plt.tight_layout()
        plt.show()
    
    def export_group_as_gif(self, group, output_path, duration=100):
        """Export a group of frames as an animated GIF"""
        images = [self.frames[frame_num]['image'] for frame_num in group 
                  if frame_num in self.frames]
        
        if images:
            images[0].save(
                output_path,
                save_all=True,
                append_images=images[1:],
                duration=duration,
                loop=0
            )
            print(f"Saved GIF to {output_path}")
    
    def generate_report(self, groups):
        """Generate a text report of the grouping"""
        print("\n" + "="*60)
        print("FRAME GROUPING REPORT")
        print("="*60)
        
        all_frame_nums = sorted(self.frames.keys())
        print(f"Total frames: {len(self.frames)}")
        print(f"Frame range: {all_frame_nums[0]} to {all_frame_nums[-1]}")
        print(f"Number of videos detected: {len(groups)}")
        print("\nVideo Details:")
        print("-"*60)
        
        for idx, group in enumerate(groups):
            print(f"Video {idx + 1}:")
            print(f"  Frames: {group[0]} to {group[-1]}")
            print(f"  Frame count: {len(group)}")
            print(f"  Duration (at 10fps): {len(group) / 10:.2f} seconds")
            print(f"  Duration (at 30fps): {len(group) / 30:.2f} seconds")
            print()


def select_frames_directory():
    """Open a GUI dialog to select the frames directory"""
    root = tk.Tk()
    root.withdraw()  # Hide the main window
    root.attributes('-topmost', True)  # Bring dialog to front
    
    messagebox.showinfo(
        "Select Frames Directory",
        "Please select the folder containing your frame_*.h files"
    )
    
    directory = filedialog.askdirectory(
        title="Select Frames Directory",
        initialdir=os.getcwd()
    )
    
    root.destroy()
    
    if not directory:
        print("No directory selected. Exiting...")
        return None
    
    # Verify the directory contains frame files
    frame_files = list(Path(directory).glob('frame_*.h'))
    if not frame_files:
        messagebox.showerror(
            "Error",
            f"No frame_*.h files found in:\n{directory}"
        )
        return None
    
    print(f"Selected directory: {directory}")
    print(f"Found {len(frame_files)} frame files")
    return directory


def select_byte_format():
    """Let user choose the correct byte format"""
    root = tk.Tk()
    root.title("Select Byte Format")
    root.geometry("500x300")
    root.attributes('-topmost', True)
    
    result = {'format': 'vertical', 'cancelled': False}
    
    def on_vertical():
        result['format'] = 'vertical'
        root.quit()
        root.destroy()
    
    def on_horizontal():
        result['format'] = 'horizontal'
        root.quit()
        root.destroy()
    
    def on_test():
        result['format'] = 'test'
        root.quit()
        root.destroy()
    
    # Main frame
    main_frame = tk.Frame(root, padx=20, pady=20)
    main_frame.pack(expand=True, fill='both')
    
    # Title
    title_label = tk.Label(
        main_frame,
        text="Select Frame Byte Format",
        font=("Arial", 14, "bold")
    )
    title_label.pack(pady=(0, 10))
    
    # Description
    desc_label = tk.Label(
        main_frame,
        text="Choose how the frame data should be interpreted.\n"
             "If unsure, select 'Test Both Formats' first.",
        justify='center'
    )
    desc_label.pack(pady=(0, 15))
    
    # Buttons
    test_btn = tk.Button(
        main_frame,
        text="Test Both Formats\n(Shows comparison)",
        command=on_test,
        width=30,
        height=2,
        bg='#4CAF50',
        fg='white'
    )
    test_btn.pack(pady=5)
    
    vertical_btn = tk.Button(
        main_frame,
        text="Vertical Page Format\n(Standard OLED SSD1306)",
        command=on_vertical,
        width=30,
        height=2
    )
    vertical_btn.pack(pady=5)
    
    horizontal_btn = tk.Button(
        main_frame,
        text="Horizontal Format\n(Row-by-row bitmap)",
        command=on_horizontal,
        width=30,
        height=2
    )
    horizontal_btn.pack(pady=5)
    
    root.mainloop()
    
    return result['format']


def get_preview_mode():
    """Get preview mode preference from user"""
    root = tk.Tk()
    root.title("Frame Preview Options")
    root.geometry("450x250")
    root.attributes('-topmost', True)
    
    result = {'mode': 'all', 'cancelled': False}
    
    def on_all():
        result['mode'] = 'all'
        root.quit()
        root.destroy()
    
    def on_paginated():
        result['mode'] = 'paginated'
        root.quit()
        root.destroy()
    
    def on_sample():
        result['mode'] = 'sample'
        root.quit()
        root.destroy()
    
    def on_skip():
        result['mode'] = 'skip'
        root.quit()
        root.destroy()
    
    # Main frame
    main_frame = tk.Frame(root, padx=20, pady=20)
    main_frame.pack(expand=True, fill='both')
    
    # Title
    title_label = tk.Label(
        main_frame,
        text="How would you like to preview frames?",
        font=("Arial", 14, "bold")
    )
    title_label.pack(pady=(0, 15))
    
    # Buttons
    btn_frame = tk.Frame(main_frame)
    btn_frame.pack(expand=True)
    
    all_btn = tk.Button(
        btn_frame,
        text="Show All Frames\n(Single scrollable view)",
        command=on_all,
        width=25,
        height=2
    )
    all_btn.pack(pady=5)
    
    paginated_btn = tk.Button(
        btn_frame,
        text="Show All Frames\n(Paginated - 50 per page)",
        command=on_paginated,
        width=25,
        height=2
    )
    paginated_btn.pack(pady=5)
    
    sample_btn = tk.Button(
        btn_frame,
        text="Show Sample Only\n(Quick preview)",
        command=on_sample,
        width=25,
        height=2
    )
    sample_btn.pack(pady=5)
    
    skip_btn = tk.Button(
        btn_frame,
        text="Skip Preview\n(Go directly to grouping)",
        command=on_skip,
        width=25,
        height=2
    )
    skip_btn.pack(pady=5)
    
    root.mainloop()
    
    return result['mode']
    """Get grouping threshold from user via GUI"""
    root = tk.Tk()
    root.title("Grouping Threshold")
    root.geometry("400x200")
    root.attributes('-topmost', True)
    
    threshold_var = tk.DoubleVar(value=15.0)
    result = {'threshold': 15.0, 'cancelled': False}
    
    def on_submit():
        try:
            result['threshold'] = threshold_var.get()
            root.quit()
            root.destroy()
        except:
            messagebox.showerror("Error", "Please enter a valid number")
    
    def on_cancel():
        result['cancelled'] = True
        root.quit()
        root.destroy()
    
    # Main frame
    main_frame = tk.Frame(root, padx=20, pady=20)
    main_frame.pack(expand=True, fill='both')
    
    # Title
    title_label = tk.Label(
        main_frame,
        text="Set Grouping Threshold",
        font=("Arial", 14, "bold")
    )
    title_label.pack(pady=(0, 10))
    
    # Description
    desc_label = tk.Label(
        main_frame,
        text="Higher value = fewer groups (more tolerant)\n"
             "Lower value = more groups (more sensitive)\n"
             "Recommended: 10-20",
        justify='left'
    )
    desc_label.pack(pady=(0, 10))
    
    # Input frame
    input_frame = tk.Frame(main_frame)
    input_frame.pack(pady=10)
    
    tk.Label(input_frame, text="Threshold:").pack(side='left', padx=(0, 5))
    threshold_entry = tk.Entry(input_frame, textvariable=threshold_var, width=10)
    threshold_entry.pack(side='left')
    
    # Buttons
    button_frame = tk.Frame(main_frame)
    button_frame.pack(pady=10)
    
    submit_btn = tk.Button(button_frame, text="OK", command=on_submit, width=10)
    submit_btn.pack(side='left', padx=5)
    
    cancel_btn = tk.Button(button_frame, text="Cancel", command=on_cancel, width=10)
    cancel_btn.pack(side='left', padx=5)
    
    root.mainloop()
    
    if result['cancelled']:
        return None
    
    return result['threshold']


def select_output_directory():
    """Select directory to save output GIFs"""
    root = tk.Tk()
    root.withdraw()
    root.attributes('-topmost', True)
    
    messagebox.showinfo(
        "Select Output Directory",
        "Please select where to save the output GIF files"
    )
    
    directory = filedialog.askdirectory(
        title="Select Output Directory",
        initialdir=os.getcwd()
    )
    
    root.destroy()
    
    if not directory:
        print("No output directory selected. Using current directory.")
        return os.getcwd()
    
    print(f"Output directory: {directory}")
    return directory


# Example usage
if __name__ == "__main__":
    # Select frames directory using GUI
    frames_directory = select_frames_directory()
    
    if not frames_directory:
        exit(1)
    
    # Select byte format
    byte_format = select_byte_format()
    
    if byte_format == 'test':
        # Test mode - show both formats for comparison
        print("Loading frames for format comparison...")
        temp_analyzer = FrameAnalyzer(frames_directory, 'vertical')
        temp_analyzer.load_all_frames()
        
        # Test with first non-empty frame
        test_frame = sorted(temp_analyzer.frames.keys())[0]
        if len(temp_analyzer.frames) > 10:
            test_frame = sorted(temp_analyzer.frames.keys())[10]  # Use frame 10 if available
        
        print(f"\nShowing format comparison for frame {test_frame}")
        print("Look at both images and determine which one looks correct.")
        temp_analyzer.test_both_formats(test_frame)
        
        # Ask user which format is correct
        byte_format = select_byte_format()
        if byte_format == 'test':
            print("Using default vertical format")
            byte_format = 'vertical'
    
    print(f"Using {byte_format} byte format")
    
    # Initialize analyzer with selected format
    analyzer = FrameAnalyzer(frames_directory, byte_format)
    
    # Load all frames
    analyzer.load_all_frames()
    
    # Get preview mode preference
    preview_mode = get_preview_mode()
    
    # Preview frames based on user selection
    if preview_mode == 'all':
        print("\nPreviewing all frames in single view...")
        analyzer.preview_frames(title='All Frames')
    elif preview_mode == 'paginated':
        print("\nPreviewing all frames (paginated)...")
        analyzer.preview_all_frames_paginated(frames_per_page=50)
    elif preview_mode == 'sample':
        print("\nPreviewing sample frames...")
        all_frames = sorted(analyzer.frames.keys())
        sample_frames = all_frames[::max(1, len(all_frames) // 30)][:30]
        analyzer.preview_frames(sample_frames, title='Sample Frames')
    else:
        print("\nSkipping preview...")
    
    # Group frames into videos (adjust threshold as needed)
    # Higher threshold = fewer groups (more tolerant to changes)
    # Lower threshold = more groups (more sensitive to changes)
    print("\nGrouping frames into videos...")
    
    # Get threshold from user
    threshold = get_threshold_value()
    if threshold is None:
        print("Threshold selection cancelled. Using default value of 15.0")
        threshold = 15.0
    
    print(f"Using threshold: {threshold}")
    groups = analyzer.group_frames_by_similarity(threshold=threshold)
    
    # Generate report
    analyzer.generate_report(groups)
    
    # Preview groups
    print("\nPreviewing video groups...")
    analyzer.preview_groups(groups)
    
    # Select output directory
    output_dir = select_output_directory()
    
    # Export each group as GIF
    print("\nExporting videos as GIFs...")
    for idx, group in enumerate(groups):
        output_file = os.path.join(output_dir, f"video_{idx + 1}.gif")
        analyzer.export_group_as_gif(group, output_file, duration=100)
        print(f"Exported Video {idx + 1} to {output_file}")
    
    # Show completion message
    root = tk.Tk()
    root.withdraw()
    root.attributes('-topmost', True)
    messagebox.showinfo(
        "Export Complete",
        f"Successfully exported {len(groups)} video(s) to:\n{output_dir}"
    )
    root.destroy()