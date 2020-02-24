import bpy

def set_environment_map(path_image):
	scene = bpy.context.scene
	world = scene.world
	world.use_nodes = True
	node_tree = world.node_tree
	environment_node = node_tree.nodes.new("ShaderNodeTexEnvironment")
	environment_node.image = bpy.data.images.load(path_image)
	node_tree.links.new(environment_node.outputs['Color'], node_tree.nodes['Background'].inputs['Color'])
