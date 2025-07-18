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
  // four cardinal neighbors
  let hL = textureLoad(heightTex, coord + vec2<i32>(-1, 0), 0).r;
  let hR = textureLoad(heightTex, coord + vec2<i32>( 1, 0), 0).r;
  let hD = textureLoad(heightTex, coord + vec2<i32>( 0,-1), 0).r;
  let hU = textureLoad(heightTex, coord + vec2<i32>( 0, 1), 0).r;
  let avg = (hL + hR + hD + hU) * 0.25;
  // blend 50% toward neighbors (thermal smoothing)
  let outH = mix(h0, avg, 0.5);
  textureStore(outputTex, coord, vec4<f32>(outH, 0.0, 0.0, 0.0));
} 