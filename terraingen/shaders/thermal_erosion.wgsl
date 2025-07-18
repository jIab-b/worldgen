@group(0) @binding(0) var heightTex : texture_2d<f32>;
@group(0) @binding(1) var outputTex : texture_storage_2d<r32float, write>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let dims = textureDimensions(heightTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  let h = textureLoad(heightTex, vec2<i32>(gid.xy), 0).r;
  // TODO: implement thermal erosion (slope-based) algorithm
  textureStore(outputTex, vec2<i32>(gid.xy), vec4<f32>(h, 0.0, 0.0, 0.0));
} 