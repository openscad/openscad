import numpy as np
import pythreejs as pjs
from ipywidgets import *

def build_widget_sub(self):
    vertices_, faces_ = self.mesh(True)

    faces = np.array(faces_)
    vertices = np.array(vertices_)
    face_verts = vertices[faces] # indices face #, face-vertex-#, coord #
    nFaces = face_verts.shape[0]
    plot_verts = face_verts.reshape((nFaces * 3, 3)).astype('float32')
    base_cols = np.tile(np.array([0.96, 0.83, 0.17]), (nFaces * 3, 1)).astype('float32')
    v1 = face_verts[:, 1, :] - face_verts[:, 0, :]
    v2 = face_verts[:, 2, :] - face_verts[:, 1, :]
    face_normals = np.cross(v1, v2)
    norm_lens = np.sqrt(np.sum(face_normals**2, axis=1)[:, np.newaxis])
    norm_lens = np.where(norm_lens > 1.0E-8, norm_lens, 1.0)
    face_normals /= norm_lens
    face_normals = np.repeat(face_normals, 3, axis=0)
    
    obj_geometry = pjs.BufferGeometry(attributes=dict(position=pjs.BufferAttribute(plot_verts), 
                                                     color=pjs.BufferAttribute(base_cols),
                                                     normal=pjs.BufferAttribute(face_normals.astype('float32'))))
        
    # Create a mesh. Note that the material need to be told to use the vertex colors.        
    my_object_mesh = pjs.Mesh(
        geometry=obj_geometry,
        material=pjs.MeshLambertMaterial(vertexColors='VertexColors'),
        position=[0, 0, 0],   
    )
        
    n_vert = vertices.shape[0]
    center = vertices.mean(axis=0)

    extents = np.zeros((3, 2))
    extents[:, 0] = np.min(vertices, axis=0)
    extents[:, 1] = np.max(vertices, axis=0)
    
    max_delta = np.max(extents[:, 1] - extents[:, 0])
    camPos = [center[i] + 4 * max_delta for i in range(3)]
    light_pos = [center[i] + (i+3)*max_delta for i in range(3)]

    # Set up a scene and render it:
    camera = pjs.PerspectiveCamera(position=camPos, fov=20,
                                   children=[pjs.DirectionalLight(color='#ffffff',
                                   position=light_pos, intensity=0.5)])
    camera.up = (0,0,1)

    v = [0.0, 0.0, 0.0]
    if n_vert > 0: v = vertices[0].tolist()
    select_point_geom = pjs.SphereGeometry(radius=1.0)
    scene_things = [my_object_mesh, camera, pjs.AmbientLight(color='#888888')]
    scene = pjs.Scene(children=scene_things, background='#ffffff')
    click_picker = pjs.Picker(controlling=my_object_mesh, event='dblclick')
    renderer_obj = pjs.Renderer(camera=camera, background='#cccc88',
        background_opacity=0, scene=scene,
        controls=[pjs.OrbitControls(controlling=camera), click_picker],
        width=640,
        height=480)
    return renderer_obj

_widget_cache = {}
def build_widget(self):
    key = id(self)
    if key in _widget_cache:
        w = _widget_cache[key]
    else:
        w =build_widget_sub(self)
        _widget_cache[key] = w
    return VBox([HTML(), w, Output()])


