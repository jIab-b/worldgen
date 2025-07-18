@group(0) @binding(0) var heightTex : texture_2d<f32>;
@group(0) @binding(1) var sdfTex : texture_storage_2d<r32float, write>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let dims = textureDimensions(sdfTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  let coord = vec2<i32>(gid.xy);
  let h = textureLoad(heightTex, coord, 0).r;
  // simple cave carve: below height threshold carve (negative), else keep empty (positive)
  let val = select(1.0, -1.0, h < 0.5);
  textureStore(sdfTex, coord, vec4<f32>(val, 0.0, 0.0, 0.0));
} 