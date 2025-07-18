@group(0) @binding(0) var heightTex : texture_2d<f32>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let dims = textureDimensions(heightTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  // stub: no-op mesh generation
} 