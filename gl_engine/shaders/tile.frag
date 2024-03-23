/*****************************************************************************
* Alpine Renderer
* Copyright (C) 2022 Adam Celarek
* Copyright (C) 2023 Gerald Kimmersdorfer
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "shared_config.glsl"
#include "camera_config.glsl"
#include "encoder.glsl"
#include "tile_id.glsl"

uniform lowp sampler2DArray ortho_sampler;
uniform highp usampler2D texture_layer_map_sampler; // todo switch to 1d textures
uniform highp usampler2D tile_id_map_sampler; // todo switch to 1d textures

layout (location = 0) out lowp vec3 texout_albedo;
layout (location = 1) out highp vec4 texout_position;
layout (location = 2) out highp uvec2 texout_normal;
layout (location = 3) out lowp vec4 texout_depth;

flat in highp int v_texture_layer;
flat in highp uvec3 var_tile_id;
in highp vec2 uv;
in highp vec3 var_pos_cws;
in highp vec3 var_normal;
#if CURTAIN_DEBUG_MODE > 0
in lowp float is_curtain;
#endif
flat in lowp vec3 vertex_color;

highp float calculate_falloff(highp float dist, highp float from, highp float to) {
    return clamp(1.0 - (dist - from) / (to - from), 0.0, 1.0);
}

highp vec3 normal_by_fragment_position_interpolation() {
    highp vec3 dFdxPos = dFdx(var_pos_cws);
    highp vec3 dFdyPos = dFdy(var_pos_cws);
    return normalize(cross(dFdxPos, dFdyPos));
}

void main() {
#if CURTAIN_DEBUG_MODE == 2
    if (is_curtain == 0.0) {
        discard;
    }
#endif

    // Write Albedo (ortho picture) in gbuffer
    highp uint hash = hash_tile_id(var_tile_id.z, var_tile_id.x, var_tile_id.y);
    highp uvec2 packed_tile_id = pack_tile_id(var_tile_id.z, var_tile_id.x, var_tile_id.y);
    while(texelFetch(tile_id_map_sampler, ivec2(int(hash & 255u), int(hash >> 8u)), 0).xy != packed_tile_id)
        hash++;

    highp float texture_layer_f = float(texelFetch(texture_layer_map_sampler, ivec2(int(hash & 255u), int(hash >> 8u)), 0).x);
    lowp vec3 fragColor = texture(ortho_sampler, vec3(uv, texture_layer_f)).rgb;
    // highp vec3 fragColor = vec3(0.0, texture_layer_f, 0.0);
    fragColor = mix(fragColor, conf.material_color.rgb, conf.material_color.a);
    texout_albedo = fragColor;

    // Write Position (and distance) in gbuffer
    highp float dist = length(var_pos_cws);
    texout_position = vec4(var_pos_cws, dist);

    // Write and encode normal in gbuffer
    highp vec3 normal = vec3(0.0);
    if (conf.normal_mode == 0u) normal = normal_by_fragment_position_interpolation();
    else normal = var_normal;
    texout_normal = octNormalEncode2u16(normal);

    // Write and encode distance for readback
    texout_depth = vec4(depthWSEncode2n8(dist), 0.0, 0.0);

    // HANDLE OVERLAYS (and mix it with the albedo color) THAT CAN JUST BE DONE IN THIS STAGE
    // (because of DATA thats not forwarded)
    // NOTE: Performancewise its generally better to handle overlays in the compose step! (screenspace effect)
    if (conf.overlay_mode > 0u && conf.overlay_mode < 100u) {
        lowp vec3 overlay_color = vec3(0.0);
        switch(conf.overlay_mode) {
            case 1u: overlay_color = normal * 0.5 + 0.5; break;
            default: overlay_color = vertex_color;
        }
        texout_albedo = mix(texout_albedo, overlay_color, conf.overlay_strength);
    }

#if CURTAIN_DEBUG_MODE == 1
    if (is_curtain > 0.0) {
        texout_albedo = vec3(1.0, 0.0, 0.0);
        return;
    }
#endif

}
