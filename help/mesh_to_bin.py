import os
import struct
import trimesh
import numpy as np


def convert_model(input_path, output_path):
    loaded = trimesh.load(input_path, force="mesh")
    if isinstance(loaded, trimesh.Scene):
        mesh = loaded.dump(concatenate=True)
    else:
        mesh = loaded

    if not hasattr(mesh, "vertices") or not hasattr(mesh, "faces") or len(mesh.vertices) == 0 or len(mesh.faces) == 0:
        raise ValueError("Model contains no vertices or faces.")

    vertices = np.asarray(mesh.vertices, dtype=np.float64)
    faces = np.asarray(mesh.faces, dtype=np.uint16)
    face_normals = np.asarray(mesh.face_normals, dtype=np.float64)

    min_val = vertices.min()
    max_val = vertices.max()
    if max_val != min_val:
        scaled_verts = ((vertices - min_val) / (max_val - min_val) * 65535 - 32768).astype(np.int16)
    else:
        scaled_verts = np.zeros_like(vertices, dtype=np.int16)

    # Quantize unit normals [-1.0, 1.0] to int16 [-32767, 32767]
    scaled_normals = np.clip(np.round(face_normals * 32767.0), -32767, 32767).astype(np.int16)

    num_verts = len(vertices)
    num_faces = len(faces)

    # Header: 2 bytes vertex count, 2 bytes face count, 4 bytes padding (Little-endian)
    header = bytearray()
    header.extend(struct.pack("<H", num_verts))
    header.extend(struct.pack("<H", num_faces))
    header.extend(b"\x00\x00\x00\x00")

    vert_data = bytearray()
    for v in scaled_verts:
        vert_data.extend(struct.pack("<3h", int(v[0]), int(v[1]), int(v[2])))

    face_data = bytearray()
    for f in faces:
        face_data.extend(struct.pack("<3H", int(f[0]), int(f[1]), int(f[2])))

    normal_data = bytearray()
    for n in scaled_normals:
        normal_data.extend(struct.pack("<3h", int(n[0]), int(n[1]), int(n[2])))

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(header)
        f.write(vert_data)
        f.write(face_data)
        f.write(normal_data)

    print(f"Converted: {input_path} -> {output_path} ({num_verts} verts, {num_faces} faces)")


def main():
    input_dir = "./source/res/raw_models"
    output_dir = "./source/res/models"

    os.makedirs(output_dir, exist_ok=True)
    supported_extensions = (".obj", ".stl", ".ply", ".gltf", ".glb", ".off", ".3mf", ".dae")

    if not os.path.exists(input_dir):
        print(f"Error: Input directory '{input_dir}' does not exist.")
        return

    converted_count = 0
    failed = []

    for root, _, files in os.walk(input_dir):
        for filename in sorted(files):
            if filename.lower().endswith(supported_extensions):
                input_path = os.path.join(root, filename)
                base_name = os.path.splitext(filename)[0]
                output_path = os.path.join(output_dir, f"{base_name}.mdl")

                try:
                    convert_model(input_path, output_path)
                    converted_count += 1
                except Exception as e:
                    failed.append((input_path, str(e)))
                    print(f"Failed to convert {input_path}: {e}")

    print(f"\nBatch conversion complete. Successfully converted {converted_count} models to '{output_dir}/'.")
    if failed:
        print(f"Failed ({len(failed)}):")
        for path, err in failed:
            print(f"  - {path}: {err}")


if __name__ == "__main__":
    main()