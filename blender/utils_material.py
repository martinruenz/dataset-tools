import bpy

# https://drafts.csswg.org/css-color-4/#named-colors
css_color_4 = {
    'aliceblue': (240, 248, 255),
    'antiquewhite': (250, 235, 215),
    'aqua': (0, 255, 255),
    'aquamarine': (127, 255, 212),
    'azure': (240, 255, 255),
    'beige': (245, 245, 220),
    'bisque': (255, 228, 196),
    'black': (0, 0, 0),
    'blanchedalmond': (255, 235, 205),
    'blue': (0, 0, 255),
    'blueviolet': (138, 43, 226),
    'brown': (165, 42, 42),
    'burlywood': (222, 184, 135),
    'cadetblue': (95, 158, 160),
    'chartreuse': (127, 255, 0),
    'chocolate': (210, 105, 30),
    'coral': (255, 127, 80),
    'cornflowerblue': (100, 149, 237),
    'cornsilk': (255, 248, 220),
    'crimson': (220, 20, 60),
    'cyan': (0, 255, 255),
    'darkblue': (0, 0, 139),
    'darkcyan': (0, 139, 139),
    'darkgoldenrod': (184, 134, 11),
    'darkgray': (169, 169, 169),
    'darkgreen': (0, 100, 0),
    'darkgrey': (169, 169, 169),
    'darkkhaki': (189, 183, 107),
    'darkmagenta': (139, 0, 139),
    'darkolivegreen': (85, 107, 47),
    'darkorange': (255, 140, 0),
    'darkorchid': (153, 50, 204),
    'darkred': (139, 0, 0),
    'darksalmon': (233, 150, 122),
    'darkseagreen': (143, 188, 143),
    'darkslateblue': (72, 61, 139),
    'darkslategray': (47, 79, 79),
    'darkslategrey': (47, 79, 79),
    'darkturquoise': (0, 206, 209),
    'darkviolet': (148, 0, 211),
    'deeppink': (255, 20, 147),
    'deepskyblue': (0, 191, 255),
    'dimgray': (105, 105, 105),
    'dimgrey': (105, 105, 105),
    'dodgerblue': (30, 144, 255),
    'firebrick': (178, 34, 34),
    'floralwhite': (255, 250, 240),
    'forestgreen': (34, 139, 34),
    'fuchsia': (255, 0, 255),
    'gainsboro': (220, 220, 220),
    'ghostwhite': (248, 248, 255),
    'gold': (255, 215, 0),
    'goldenrod': (218, 165, 32),
    'gray': (128, 128, 128),
    'green': (0, 128, 0),
    'greenyellow': (173, 255, 47),
    'grey': (128, 128, 128),
    'honeydew': (240, 255, 240),
    'hotpink': (255, 105, 180),
    'indianred': (205, 92, 92),
    'indigo': (75, 0, 130),
    'ivory': (255, 255, 240),
    'khaki': (240, 230, 140),
    'lavender': (230, 230, 250),
    'lavenderblush': (255, 240, 245),
    'lawngreen': (124, 252, 0),
    'lemonchiffon': (255, 250, 205),
    'lightblue': (173, 216, 230),
    'lightcoral': (240, 128, 128),
    'lightcyan': (224, 255, 255),
    'lightgoldenrodyellow': (250, 250, 210),
    'lightgray': (211, 211, 211),
    'lightgreen': (144, 238, 144),
    'lightgrey': (211, 211, 211),
    'lightpink': (255, 182, 193),
    'lightsalmon': (255, 160, 122),
    'lightseagreen': (32, 178, 170),
    'lightskyblue': (135, 206, 250),
    'lightslategray': (119, 136, 153),
    'lightslategrey': (119, 136, 153),
    'lightsteelblue': (176, 196, 222),
    'lightyellow': (255, 255, 224),
    'lime': (0, 255, 0),
    'limegreen': (50, 205, 50),
    'linen': (250, 240, 230),
    'magenta': (255, 0, 255),
    'maroon': (128, 0, 0),
    'mediumaquamarine': (102, 205, 170),
    'mediumblue': (0, 0, 205),
    'mediumorchid': (186, 85, 211),
    'mediumpurple': (147, 112, 219),
    'mediumseagreen': (60, 179, 113),
    'mediumslateblue': (123, 104, 238),
    'mediumspringgreen': (0, 250, 154),
    'mediumturquoise': (72, 209, 204),
    'mediumvioletred': (199, 21, 133),
    'midnightblue': (25, 25, 112),
    'mintcream': (245, 255, 250),
    'mistyrose': (255, 228, 225),
    'moccasin': (255, 228, 181),
    'navajowhite': (255, 222, 173),
    'navy': (0, 0, 128),
    'oldlace': (253, 245, 230),
    'olive': (128, 128, 0),
    'olivedrab': (107, 142, 35),
    'orange': (255, 165, 0),
    'orangered': (255, 69, 0),
    'orchid': (218, 112, 214),
    'palegoldenrod': (238, 232, 170),
    'palegreen': (152, 251, 152),
    'paleturquoise': (175, 238, 238),
    'palevioletred': (219, 112, 147),
    'papayawhip': (255, 239, 213),
    'peachpuff': (255, 218, 185),
    'peru': (205, 133, 63),
    'pink': (255, 192, 203),
    'plum': (221, 160, 221),
    'powderblue': (176, 224, 230),
    'purple': (128, 0, 128),
    'rebeccapurple': (102, 51, 153),
    'red': (255, 0, 0),
    'rosybrown': (188, 143, 143),
    'royalblue': (65, 105, 225),
    'saddlebrown': (139, 69, 19),
    'salmon': (250, 128, 114),
    'sandybrown': (244, 164, 96),
    'seagreen': (46, 139, 87),
    'seashell': (255, 245, 238),
    'sienna': (160, 82, 45),
    'silver': (192, 192, 192),
    'skyblue': (135, 206, 235),
    'slateblue': (106, 90, 205),
    'slategray': (112, 128, 144),
    'slategrey': (112, 128, 144),
    'snow': (255, 250, 250),
    'springgreen': (0, 255, 127),
    'steelblue': (70, 130, 180),
    'tan': (210, 180, 140),
    'teal': (0, 128, 128),
    'thistle': (216, 191, 216),
    'tomato': (255, 99, 71),
    'turquoise': (64, 224, 208),
    'violet': (238, 130, 238),
    'wheat': (245, 222, 179),
    'white': (255, 255, 255),
    'whitesmoke': (245, 245, 245),
    'yellow': (255, 255, 0),
    'yellowgreen': (154, 205, 50)
}

def get_colorramp_node(parent_node_tree, map='black_white', name=None):
    node = parent_node_tree.nodes.new(type="ShaderNodeValToRGB")
    node.color_ramp.color_mode = 'RGB'
    node.color_ramp.interpolation = 'EASE'
    if name is not None:
        node.name = name
    assert len(node.color_ramp.elements) == 2
    if map == 'bgr':
        node.color_ramp.elements.new(position=0.5)
        node.color_ramp.elements[0].color = (0.0, 0.368, 1.0, 1.0)
        node.color_ramp.elements[1].color = (0.0016, 0.5, 0.0, 1.0)
        node.color_ramp.elements[2].color = (1.0, 0.0, 0.0016, 1.0)
    return node


def get_material_heightmap(colormap='bgr', axis='Z'):
    mat = bpy.data.materials.new(name='bgr-heightmap-'+axis)
    mat.use_nodes = True
    node_tree = mat.node_tree
    links = node_tree.links
    node_in = node_tree.nodes.new(type="ShaderNodeTexCoord")
    node_in.location.xy = (0, 0)
    node_separate = node_tree.nodes.new(type="ShaderNodeSeparateXYZ")
    node_separate.location.xy = (250, 0)
    node_ramp = get_colorramp_node(parent_node_tree=mat.node_tree, map=colormap)
    node_ramp.location.xy = (500, 0)
    node_bsdf = node_tree.nodes['Principled BSDF']
    node_bsdf.location.xy = (850, 0)
    node_out = node_tree.nodes['Material Output']
    node_out.location.xy = (1200, 0)
    links.new(node_in.outputs['Generated'], node_separate.inputs['Vector'])
    links.new(node_separate.outputs[axis], node_ramp.inputs['Fac'])
    links.new(node_ramp.outputs['Color'], node_bsdf.inputs['Base Color'])
    links.new(node_bsdf.outputs['BSDF'], node_out.inputs['Surface'])
    return mat


def get_material_checker(color0=(0, 0, 0, 1), color1=(1, 1, 1, 1), scale=1.0, **kwargs):
    mat = bpy.data.materials.new(name='checker')
    mat.use_nodes = True
    mat_nodes = mat.node_tree.nodes
    mat_links = mat.node_tree.links
    mat_nodes.clear()

    node_uv_checker = mat_nodes.new(type="ShaderNodeTexCoord")
    node_tex_checker = mat_nodes.new(type="ShaderNodeTexChecker")
    node_tex_checker.inputs['Color1'].default_value = color0
    node_tex_checker.inputs['Color2'].default_value = color1
    node_tex_checker.inputs['Scale'].default_value = scale
    node_diffuse_checker = mat_nodes.new(type="ShaderNodeBsdfDiffuse")
    node_output_checker = mat_nodes.new(type="ShaderNodeOutputMaterial")

    # mat_checker_links.new(node_uv_checker.outputs['UV'], node_tex_checker.inputs[0])
    mat_links.new(node_tex_checker.outputs['Color'], node_diffuse_checker.inputs[0])
    mat_links.new(node_diffuse_checker.outputs['BSDF'], node_output_checker.inputs[0])

    return mat


def get_material(material, name=None, **kwargs):
    if type(material) == bpy.types.Material:
        return material
    if type(material) == str:
        material = css_color_4[material.lower()]
    if type(material) == tuple:
        if max(material) > 1:
            material = tuple(c / 255 for c in material)
        if len(material) < 4:
            material = (material[0], material[1], material[2], 1)
        if name is None:
            name = 'color-{:.3f}-{:.3f}-{:.3f}-{:.3f}'.format(*material)
        if name in bpy.data.materials:
            return bpy.data.materials[name]
        mat = bpy.data.materials.new(name=name)
        mat.use_nodes = True
        shader = mat.node_tree.nodes['Principled BSDF']
        shader.inputs[0].default_value = material
        return mat
    RuntimeError('Color not understood')
