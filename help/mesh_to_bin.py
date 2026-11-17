import os
import struct
import trimesh
import numpy as np

def convert_model(input_path, output_path):
    mesh = trimesh.load(input_path)
    if isinstance(mesh, trimesh.Scene):
        mesh = mesh.dump(concatenate=True)

    vertices = mesh.vertices
    faces = mesh.faces

    min_val = vertices.min()
    max_val = vertices.max()
    if max_val != min_val:
        scaled_verts = ((vertices - min_val) / (max_val - min_val) * 65535 - 32768).astype(np.int16)
    else:
        scaled_verts = np.zeros_like(vertices, dtype=np.int16)

    num_verts = len(vertices)
    num_faces = len(faces)

    # Header: 2 bytes vertex count, 2 bytes face count, 4 bytes padding (Little-endian)
    header = bytearray()
    header.extend(struct.pack('<H', num_verts))
    header.extend(struct.pack('<H', num_faces))
    header.extend(b'\x00\x00\x00\x00')

    vert_data = bytearray()
    for v in scaled_verts:
        vert_data.extend(struct.pack('<3h', v[0], v[1], v[2]))

    face_data = bytearray()
    for f in faces:
        face_data.extend(struct.pack('<3H', int(f[0]), int(f[1]), int(f[2])))

    # Write binary file
    with open(output_path, "wb") as f:
        f.write(header)
        f.write(vert_data)
        f.write(face_data)

    print(f"Converted: {os.path.basename(input_path)} -> {output_path} ({num_verts} verts, {num_faces} faces)")

def main():
    input_dir = "./source/res/raw_models"
    output_dir = "./source/res/models"

    os.makedirs(output_dir, exist_ok=True)
    supported_extensions = ('.obj', '.stl', '.ply', '.gltf', '.glb')

    if not os.path.exists(input_dir):
        print(f"Error: Input directory '{input_dir}' does not exist.")
        return

    count = 0
    for filename in os.listdir(input_dir):
        if filename.lower().endswith(supported_extensions):
            input_path = os.path.join(input_dir, filename)
            base_name = os.path.splitext(filename)[0]
            output_path = os.path.join(output_dir, f"{base_name}.bin")
            
            try:
                convert_model(input_path, output_path)
                count += 1
            except Exception as e:
                print(f"Failed to convert {filename}: {e}")

    print(f"\nBatch conversion complete. Successfully converted {count} models to '{output_dir}/'.")

if __name__ == "__main__":
    main()