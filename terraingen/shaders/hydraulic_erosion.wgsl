@group(0) @binding(0) var heightTex : texture_2d<f32>;
@group(0) @binding(1) var outputTex : texture_storage_2d<r32float, write>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let dims = textureDimensions(heightTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  let coord = vec2<i32>(gid.xy);
  let h0 = textureLoad(heightTex, coord, 0).r;
  var sum: f32 = 0.0;
  var count: i32 = 0;
  // 3x3 neighbor average
  for (var dy: i32 = -1; dy <= 1; dy = dy + 1) {
    for (var dx: i32 = -1; dx <= 1; dx = dx + 1) {
      let nc = coord + vec2<i32>(dx, dy);
      if (nc.x >= 0 && nc.y >= 0 && u32(nc.x) < dims.x && u32(nc.y) < dims.y) {
        sum = sum + textureLoad(heightTex, nc, 0).r;
        count = count + 1;
      }
    }
  }
  let avg = sum / f32(count);
  // move 10% toward average (erode)
  let outH = h0 + (avg - h0) * 0.1;
  textureStore(outputTex, coord, vec4<f32>(outH, 0.0, 0.0, 0.0));
} 