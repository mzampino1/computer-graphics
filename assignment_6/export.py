import bpy

# Export selected object

def write_some_data(context, filepath, use_some_setting):
    print("running write_some_data...")
    f = open(filepath, "w", encoding='utf-8')
    context.active_object.data.calc_loop_triangles()
    
    f.write(str(len(context.active_object.data.loop_triangles)*3) + '\n')
    for triangle in context.active_object.data.loop_triangles:
        for vertex_index in triangle.vertices:
            vertex = context.active_object.data.vertices[vertex_index]
            f.write(str(float(vertex.co.x)) + ' ' + str(float(vertex.co.z)) + ' ' + str(float(-vertex.co.y))+ '\n')
    for triangle in context.active_object.data.loop_triangles:
        for vertex_index in triangle.vertices:
            vertex = context.active_object.data.vertices[vertex_index]
            f.write(str(float(vertex.normal.x)) + ' ' + str(float(vertex.normal.z)) + ' ' + str(float(-vertex.normal.y)) + '\n')
        
    for triangle in context.active_object.data.loop_triangles:
        for loop_index in triangle.loops:
            data_element = context.active_object.data.uv_layers[0].data[loop_index]
            f.write(str(float(data_element.uv.x)) + ' ' + str(float(-data_element.uv.y)) + '\n')
        
    f.close()

    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportSomeData(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_test.some_data"  # Important since its how bpy.ops.import_test.some_data is constructed.
    bl_label = "Export Some Data"

    # ExportHelper mix-in class uses this.
    filename_ext = ".txt"

    filter_glob: StringProperty(
        default="*.txt",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    use_setting: BoolProperty(
        name="Example Boolean",
        description="Example Tooltip",
        default=True,
    )

    type: EnumProperty(
        name="Example Enum",
        description="Choose between two items",
        items=(
            ('OPT_A', "First Option", "Description one"),
            ('OPT_B', "Second Option", "Description two"),
        ),
        default='OPT_A',
    )

    def execute(self, context):
        return write_some_data(context, self.filepath, self.use_setting)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="Text Export Operator")


# Register and add to the "file selector" menu (required to use F3 search "Text Export Operator" for quick access).
def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # Test call.
    bpy.ops.export_test.some_data('INVOKE_DEFAULT')
