@group(0) @binding(0) var heightTex : texture_2d<f32>;
@group(0) @binding(1) var outputAlbedo : texture_storage_2d<r32float, write>;
@group(0) @binding(2) var outputNormal : texture_storage_2d<r32float, write>;
@group(0) @binding(3) var outputRoughness : texture_storage_2d<r32float, write>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let dims = textureDimensions(outputAlbedo);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  let coord = vec2<i32>(gid.xy);
  let h = textureLoad(heightTex, coord, 0).r;
  // simple stub: albedo and roughness follow height, normal is unit Y
  textureStore(outputAlbedo, coord, vec4<f32>(h, 0.0, 0.0, 0.0));
  textureStore(outputNormal, coord, vec4<f32>(1.0, 0.0, 0.0, 0.0));
  textureStore(outputRoughness, coord, vec4<f32>(h, 0.0, 0.0, 0.0));
} 