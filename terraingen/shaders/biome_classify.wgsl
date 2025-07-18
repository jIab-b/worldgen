@group(0) @binding(0) var<uniform> thresholds : vec2<f32>;
@group(0) @binding(1) var heightTex : texture_2d<f32>;
@group(0) @binding(2) var outputTex : texture_storage_2d<r8uint, write>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let dims = textureDimensions(heightTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  let h = textureLoad(heightTex, vec2<i32>(gid.xy), 0).r;
  var id: u32 = 0u;
  if (h >= thresholds.x) { id = 1u; }
  if (h >= thresholds.y) { id = 2u; }
  textureStore(outputTex, vec2<i32>(gid.xy), vec4<u32>(id, 0u, 0u, 1u));
} 