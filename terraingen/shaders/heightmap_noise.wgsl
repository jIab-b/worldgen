@group(0) @binding(0) var outputTex : texture_storage_2d<r32float, write>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let dims : vec2<u32> = textureDimensions(outputTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  let x : f32 = f32(gid.x);
  let y : f32 = f32(gid.y);
  // value noise via sin-based PRNG for FBM base
  let v : f32 = fract(sin((x*12.9898 + y*78.233)) * 43758.5453);
  textureStore(outputTex, vec2<i32>(gid.xy), vec4<f32>(v, 0.0, 0.0, 0.0));
} 