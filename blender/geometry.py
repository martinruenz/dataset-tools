import bpy


def create_plane_meshdata(w, h, uv_border=0, **kwargs):
    verts = [
        (-0.5 * w, -0.5 * h, 0.0),
        (0.5 * w, -0.5 * h, 0.0),
        (0.5 * w, 0.5 * h, 0.0),
        (-0.5 * w, 0.5 * h, 0.0)
    ]
    faces = [(0, 1, 2, 3)]
    mesh_data = bpy.data.meshes.new("plane")
    mesh_data.from_pydata(verts, [], faces)

    if uv_border >= 0:
        border_width_u = uv_border
        border_width_v = border_width_u * w / h
        mesh_data.uv_layers.new()
        mesh_data.uv_layers.active.data[0].uv = (-border_width_u, -border_width_v)
        mesh_data.uv_layers.active.data[1].uv = (1 + border_width_u, -border_width_v)
        mesh_data.uv_layers.active.data[2].uv = (1 + border_width_u, 1 + border_width_v)
        mesh_data.uv_layers.active.data[3].uv = (-border_width_u, 1 + border_width_v)

    mesh_data.update()
    return mesh_data