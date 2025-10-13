import os
import re
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
from pathlib import Path
from collections import defaultdict
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog
from tkinter import ttk

class FrameAnalyzer:
    def __init__(self, frames_directory, byte_format='horizontal'):
        self.frames_dir = Path(frames_directory)
        self.frames = {}
        self.width = 128
        self.height = 64
        self.byte_format = byte_format
        
    def parse_frame_file(self, filepath):
        """Parse a .h file and extract bitmap data"""
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Extract frame number from filename
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
    
    def bytes_to_image_horizontal(self, byte_array):
        """Convert byte array with horizontal byte ordering"""
        img_array = np.zeros((self.height, self.width), dtype=np.uint8)
        
        byte_idx = 0
        for y in range(self.height):
            for x_byte in range(self.width // 8):
                if byte_idx < len(byte_array):
                    byte_val = byte_array[byte_idx]
                    for bit in range(8):
                        x = x_byte * 8 + bit
                        if byte_val & (1 << (7 - bit)):
                            img_array[y, x] = 255
                    byte_idx += 1
        
        return Image.fromarray(img_array, mode='L')
    
    def load_all_frames(self):
        """Load all frame files from directory"""
        frame_files = list(self.frames_dir.glob('frame_*.h'))
        if not frame_files:
            frame_files = list(self.frames_dir.glob('frame*.h'))
        
        def get_frame_num(filepath):
            match = re.search(r'frame[_-]?0*(\d+)', filepath.name)
            return int(match.group(1)) if match else 0
        
        frame_files.sort(key=get_frame_num)
        
        print(f"Found {len(frame_files)} frame files")
        print("Loading frames", end='', flush=True)
        
        for i, filepath in enumerate(frame_files):
            result = self.parse_frame_file(filepath)
            if result:
                frame_num, byte_array = result
                img = self.bytes_to_image_horizontal(byte_array)
                    
                self.frames[frame_num] = {
                    'image': img,
                    'data': byte_array,
                    'path': filepath
                }
            
            if (i + 1) % 50 == 0:
                print('.', end='', flush=True)
        
        print()
        print(f"Successfully loaded {len(self.frames)} frames")
        return self.frames
    
    def preview_frames(self, frame_numbers=None, rows=None, cols=None, title='Frame Preview'):
        """Preview selected frames in a grid"""
        if not self.frames:
            self.load_all_frames()
        
        if frame_numbers is None:
            frame_numbers = sorted(self.frames.keys())
        
        if rows is None or cols is None:
            total_frames = len(frame_numbers)
            if total_frames <= 24:
                cols = 6
                rows = (total_frames + cols - 1) // cols
            elif total_frames <= 100:
                cols = 10
                rows = (total_frames + cols - 1) // cols
            else:
                cols = int(np.ceil(np.sqrt(total_frames)))
                rows = (total_frames + cols - 1) // cols
        
        fig_width = min(20, cols * 2)
        fig_height = min(30, rows * 1.5)
        
        fig, axes = plt.subplots(rows, cols, figsize=(fig_width, fig_height))
        fig.suptitle(title, fontsize=16)
        
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
        
        for idx in range(len(frame_numbers), len(axes)):
            axes[idx].axis('off')
        
        plt.tight_layout()
        plt.show()
    
    def export_to_header_file(self, frame_numbers, output_path, animation_name, fps=10):
        """Export selected frames to Arduino .h file format"""
        
        # Generate the header guard name
        guard_name = animation_name.upper() + "_H"
        
        # Calculate metadata
        frame_count = len(frame_numbers)
        bytes_per_frame = 1024  # 128 * 8 bytes for horizontal format
        total_size = frame_count * bytes_per_frame
        frame_delay = int(1000 / fps)
        
        with open(output_path, 'w') as f:
            # Write header
            f.write(f"#ifndef {guard_name}\n")
            f.write(f"#define {guard_name}\n\n")
            
            f.write(f"// Auto-generated video data for {animation_name}\n")
            f.write(f"// Frame count: {frame_count}\n")
            f.write(f"// Frame rate: {fps} FPS\n")
            f.write(f"// Bytes per frame: {bytes_per_frame}\n")
            f.write(f"// Total size: {total_size} bytes\n\n")
            
            f.write("#include <Arduino.h>\n\n")
            
            # Write metadata constants
            f.write(f"const int {animation_name}_FRAME_COUNT = {frame_count};\n")
            f.write(f"const int {animation_name}_FPS = {fps};\n")
            f.write(f"const int {animation_name}_FRAME_DELAY = {frame_delay}; // milliseconds\n\n")
            
            # Write frame data array
            f.write(f"const uint8_t PROGMEM {animation_name}_frames[][{bytes_per_frame}] = {{\n")
            
            for idx, frame_num in enumerate(frame_numbers):
                if frame_num not in self.frames:
                    print(f"Warning: Frame {frame_num} not found, skipping")
                    continue
                
                byte_array = self.frames[frame_num]['data']
                
                # Write frame opening
                f.write("  {\n")
                
                # Write bytes in rows of 16
                for i in range(0, len(byte_array), 16):
                    chunk = byte_array[i:i+16]
                    hex_values = ','.join(f'0x{b:02X}' for b in chunk)
                    
                    # Add comma at end if not last line
                    if i + 16 < len(byte_array):
                        f.write(f"    {hex_values},\n")
                    else:
                        f.write(f"    {hex_values}\n")
                
                # Write frame closing
                if idx < len(frame_numbers) - 1:
                    f.write("  },\n")
                else:
                    f.write("  }\n")
            
            f.write("};\n\n")
            f.write(f"#endif // {guard_name}\n")
        
        print(f"\nHeader file saved to: {output_path}")
        print(f"Animation name: {animation_name}")
        print(f"Frames included: {frame_count}")


class ManualFrameSelector:
    def __init__(self, analyzer):
        self.analyzer = analyzer
        self.selected_frames = []
        self.root = None
        
    def show_selector_gui(self):
        """Show GUI for manual frame selection"""
        self.root = tk.Tk()
        self.root.title("Manual Frame Selection")
        self.root.geometry("600x700")
        
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Title
        title_label = ttk.Label(main_frame, text="Select Frames for Animation", 
                               font=("Arial", 14, "bold"))
        title_label.grid(row=0, column=0, columnspan=2, pady=10)
        
        # Available frames display
        info_label = ttk.Label(main_frame, 
                              text=f"Available frames: {min(self.analyzer.frames.keys())} to {max(self.analyzer.frames.keys())}")
        info_label.grid(row=1, column=0, columnspan=2, pady=5)
        
        # Input method selection
        method_frame = ttk.LabelFrame(main_frame, text="Selection Method", padding="10")
        method_frame.grid(row=2, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        
        # Range selection
        ttk.Label(method_frame, text="Frame Range:").grid(row=0, column=0, sticky=tk.W)
        ttk.Label(method_frame, text="From:").grid(row=1, column=0, sticky=tk.E, padx=5)
        self.start_entry = ttk.Entry(method_frame, width=10)
        self.start_entry.grid(row=1, column=1, padx=5)
        
        ttk.Label(method_frame, text="To:").grid(row=1, column=2, sticky=tk.E, padx=5)
        self.end_entry = ttk.Entry(method_frame, width=10)
        self.end_entry.grid(row=1, column=3, padx=5)
        
        ttk.Button(method_frame, text="Add Range", 
                  command=self.add_range).grid(row=1, column=4, padx=5)
        
        # Individual frame selection
        ttk.Label(method_frame, text="Individual Frames:").grid(row=2, column=0, sticky=tk.W, pady=(10,0))
        ttk.Label(method_frame, text="Frames (comma-separated):").grid(row=3, column=0, sticky=tk.E, padx=5)
        self.frames_entry = ttk.Entry(method_frame, width=30)
        self.frames_entry.grid(row=3, column=1, columnspan=3, sticky=(tk.W, tk.E), padx=5)
        
        ttk.Button(method_frame, text="Add Frames", 
                  command=self.add_individual).grid(row=3, column=4, padx=5)
        
        # Selected frames display
        display_frame = ttk.LabelFrame(main_frame, text="Selected Frames", padding="10")
        display_frame.grid(row=3, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Scrollable text widget
        self.selected_text = tk.Text(display_frame, height=15, width=60)
        scrollbar = ttk.Scrollbar(display_frame, orient="vertical", command=self.selected_text.yview)
        self.selected_text.configure(yscrollcommand=scrollbar.set)
        
        self.selected_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Control buttons
        button_frame = ttk.Frame(main_frame)
        button_frame.grid(row=4, column=0, columnspan=2, pady=10)
        
        ttk.Button(button_frame, text="Preview Selection", 
                  command=self.preview_selection).grid(row=0, column=0, padx=5)
        ttk.Button(button_frame, text="Clear All", 
                  command=self.clear_selection).grid(row=0, column=1, padx=5)
        ttk.Button(button_frame, text="Export to .h File", 
                  command=self.export_to_header).grid(row=0, column=2, padx=5)
        
        # Status label
        self.status_label = ttk.Label(main_frame, text="", foreground="blue")
        self.status_label.grid(row=5, column=0, columnspan=2, pady=5)
        
        self.root.mainloop()
    
    def add_range(self):
        """Add a range of frames"""
        try:
            start = int(self.start_entry.get())
            end = int(self.end_entry.get())
            
            if start > end:
                messagebox.showerror("Error", "Start frame must be less than or equal to end frame")
                return
            
            available_frames = sorted(self.analyzer.frames.keys())
            for frame_num in range(start, end + 1):
                if frame_num in available_frames and frame_num not in self.selected_frames:
                    self.selected_frames.append(frame_num)
            
            self.selected_frames.sort()
            self.update_display()
            self.start_entry.delete(0, tk.END)
            self.end_entry.delete(0, tk.END)
            
        except ValueError:
            messagebox.showerror("Error", "Please enter valid frame numbers")
    
    def add_individual(self):
        """Add individual frames"""
        try:
            frames_text = self.frames_entry.get()
            frame_list = [int(f.strip()) for f in frames_text.split(',') if f.strip()]
            
            available_frames = sorted(self.analyzer.frames.keys())
            for frame_num in frame_list:
                if frame_num in available_frames and frame_num not in self.selected_frames:
                    self.selected_frames.append(frame_num)
            
            self.selected_frames.sort()
            self.update_display()
            self.frames_entry.delete(0, tk.END)
            
        except ValueError:
            messagebox.showerror("Error", "Please enter valid frame numbers separated by commas")
    
    def update_display(self):
        """Update the selected frames display"""
        self.selected_text.delete(1.0, tk.END)
        
        if self.selected_frames:
            text = f"Total frames selected: {len(self.selected_frames)}\n\n"
            text += "Frame numbers:\n"
            
            # Display in rows of 10
            for i in range(0, len(self.selected_frames), 10):
                chunk = self.selected_frames[i:i+10]
                text += ', '.join(map(str, chunk)) + '\n'
            
            self.selected_text.insert(1.0, text)
            self.status_label.config(text=f"{len(self.selected_frames)} frames selected")
        else:
            self.selected_text.insert(1.0, "No frames selected")
            self.status_label.config(text="")
    
    def clear_selection(self):
        """Clear all selected frames"""
        self.selected_frames = []
        self.update_display()
    
    def preview_selection(self):
        """Preview the selected frames"""
        if not self.selected_frames:
            messagebox.showwarning("Warning", "No frames selected")
            return
        
        self.analyzer.preview_frames(self.selected_frames, title="Selected Frames Preview")
    
    def export_to_header(self):
        """Export selected frames to .h file"""
        if not self.selected_frames:
            messagebox.showwarning("Warning", "No frames selected")
            return
        
        # Ask for animation name
        animation_name = simpledialog.askstring("Animation Name", 
                                               "Enter animation name (e.g., 'chotu_patting'):",
                                               parent=self.root)
        if not animation_name:
            return
        
        # Clean the animation name
        animation_name = re.sub(r'[^a-zA-Z0-9_]', '_', animation_name)
        
        # Ask for FPS
        fps = simpledialog.askinteger("Frame Rate", 
                                     "Enter frame rate (FPS):",
                                     initialvalue=10,
                                     minvalue=1,
                                     maxvalue=60,
                                     parent=self.root)
        if not fps:
            fps = 10
        
        # Select output file
        output_file = filedialog.asksaveasfilename(
            title="Save Header File",
            defaultextension=".h",
            filetypes=[("Header files", "*.h"), ("All files", "*.*")],
            initialfile=f"{animation_name}.h"
        )
        
        if not output_file:
            return
        
        # Export
        self.analyzer.export_to_header_file(self.selected_frames, output_file, animation_name, fps)
        
        messagebox.showinfo("Success", 
                          f"Header file created successfully!\n\n"
                          f"File: {output_file}\n"
                          f"Frames: {len(self.selected_frames)}\n"
                          f"FPS: {fps}")


def main():
    # Select frames directory
    root = tk.Tk()
    root.withdraw()
    root.attributes('-topmost', True)
    
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
        return
    
    # Verify frames exist
    frame_files = list(Path(directory).glob('frame_*.h'))
    if not frame_files:
        messagebox.showerror("Error", f"No frame_*.h files found in:\n{directory}")
        return
    
    print(f"Selected directory: {directory}")
    print(f"Found {len(frame_files)} frame files")
    
    # Initialize analyzer with horizontal format
    print("\nUsing Horizontal byte format (as confirmed)")
    analyzer = FrameAnalyzer(directory, byte_format='horizontal')
    
    # Load all frames
    analyzer.load_all_frames()
    
    # Show selection mode choice
    root = tk.Tk()
    root.title("Selection Mode")
    root.geometry("400x200")
    root.attributes('-topmost', True)
    
    selection_mode = {'mode': None}
    
    def manual_mode():
        selection_mode['mode'] = 'manual'
        root.quit()
        root.destroy()
    
    def preview_first():
        selection_mode['mode'] = 'preview'
        root.quit()
        root.destroy()
    
    main_frame = tk.Frame(root, padx=20, pady=20)
    main_frame.pack(expand=True, fill='both')
    
    tk.Label(main_frame, text="How would you like to proceed?", 
            font=("Arial", 12, "bold")).pack(pady=(0, 20))
    
    tk.Button(main_frame, text="Manual Frame Selection", 
             command=manual_mode, width=25, height=2).pack(pady=5)
    
    tk.Button(main_frame, text="Preview Frames First", 
             command=preview_first, width=25, height=2).pack(pady=5)
    
    root.mainloop()
    
    if selection_mode['mode'] == 'preview':
        print("\nPreviewing sample frames...")
        all_frames = sorted(analyzer.frames.keys())
        sample_frames = all_frames[::max(1, len(all_frames) // 30)][:30]
        analyzer.preview_frames(sample_frames, title='Sample Frames')
        selection_mode['mode'] = 'manual'
    
    if selection_mode['mode'] == 'manual':
        print("\nOpening manual frame selector...")
        selector = ManualFrameSelector(analyzer)
        selector.show_selector_gui()


if __name__ == "__main__":
    main()