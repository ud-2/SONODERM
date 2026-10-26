#version 330 core

out vec4 frag_color;

uniform vec2 u_resolution;

// Temporal Feedback: previous frame texture
uniform sampler2D u_prev_frame;

// Dynamic Audio Uniforms
uniform float u_audio_time;
uniform float u_bass_impact;
uniform float u_mid_swell;
uniform float u_treble_spark;
uniform float u_energy;

// Ghost UI & Playback state
uniform float u_progress; // Track progress [0.0, 1.0]
uniform float u_is_paused; // 1.0 = Paused, 0.0 = Playing

// Song DNA Uniforms: Palette
uniform vec3 u_pal_a;
uniform vec3 u_pal_b;
uniform vec3 u_pal_c;
uniform vec3 u_pal_d;

// Song DNA Uniforms: Morphology
uniform float u_shape_type;
uniform float u_satellites;
uniform float u_twist;
uniform float u_roughness;

/**
 * @brief Calculates the scene palette based on a time parameter
 *
 * @see https://iquilezles.org/articles/palettes/
 */
vec3 get_scene_palette(float t) {
    const float TAU = 6.28318530718; // 2 * PI

    return u_pal_a + u_pal_b * cos(TAU * (u_pal_c * t + u_pal_d));
}

// Smooth Minimum: merges two distances a and b with a blending radius k
float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// Signed Distance Functions (SDFs) for 3D shapes

float sd_sphere(vec3 p, float r) {
    return length(p) - r;
}

float sd_round_box(vec3 p, vec3 b, float r) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - r;
}

float sd_octahedron(vec3 p, float s) {
    p = abs(p);
    return (p.x + p.y + p.z - s) * 0.57735027;
}

float sd_torus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

// 3D twist: apply a 2D rotation matrix on the XZ plan depending on Y height
vec3 twist_space(vec3 p, float k) {
    float c = cos(k * p.y);
    float s = sin(k * p.y);
    mat2 m = mat2(c, -s, s, c);
    return vec3(m * p.xz, p.y).xzy;
}

// Scene Description
float map(vec3 p) {
    // twist along with mid
    vec3 p_twisted = twist_space(p, u_twist * (1.0 + u_mid_swell * 0.5));

    // expand radius along with bass
    float core = 0.0;
    float growth = u_bass_impact * 0.28;

    if (u_shape_type < 0.5) {
        core = sd_sphere(p_twisted, 0.75 + growth);
    } else if (u_shape_type < 1.5) {
        core = sd_round_box(p_twisted, vec3(0.55 + growth * 0.5), 0.2);
    } else {
        core = sd_octahedron(p_twisted, 0.95 + growth);
    }

    // rotate satellites
    float living_matter = core;
    float sat_speed = u_audio_time * 1.3;

    for (int i = 0; i < 3; i++) {
        if (float(i) >= u_satellites) break;
        float angle_offset = float(i) * 2.09439;
        vec3 sat_pos = p - vec3(sin(sat_speed + angle_offset) * 1.4, 
                                cos(sat_speed * 0.8 + angle_offset) * 0.9, 
                                sin(sat_speed * 0.5) * 0.5);
        float sat = sd_sphere(sat_pos, 0.25 + u_mid_swell * 0.08);
        living_matter = smin(living_matter, sat, 0.35);
    }

    // rotating ring
    float ring_angle = u_audio_time * 0.55;
    mat2 rot = mat2(cos(ring_angle), -sin(ring_angle), sin(ring_angle), cos(ring_angle));
    vec3 p_ring = p;
    p_ring.xz = rot * p_ring.xz;
    p_ring.yz = rot * p_ring.yz;
    float ring = sd_torus(p_ring, vec2(1.4, 0.09 + u_mid_swell * 0.1));

    float fusion_k = 0.3 + (u_energy * 0.15);
    living_matter = smin(living_matter, ring, fusion_k);

    // ripples along with treble (to modify roughness)
    float rip_freq = 10.0 * u_roughness;
    float rip_amp  = 0.015 + (u_treble_spark * 0.045);
    float ripples = sin(p.x * rip_freq + u_audio_time * 4.0) 
                  * sin(p.y * rip_freq + u_audio_time * 4.0) 
                  * sin(p.z * rip_freq + u_audio_time * 4.0) * rip_amp;

    return living_matter + ripples;
}

// just calculate gradient on every 3D ax
vec3 calculate_normal(vec3 p) {
    const float eps = 0.001;
    const vec2 h = vec2(eps, 0.0);
    return normalize(vec3(
        map(p + h.xyy) - map(p - h.xyy),
        map(p + h.yxy) - map(p - h.yxy),
        map(p + h.yyx) - map(p - h.yyx)
    ));
}

float raymarch(vec3 ro, vec3 rd) {
    float t = 0.0;
    for (int i = 0; i < 90; i++) {
        vec3 p = ro + rd * t;

        float d = map(p);
        if (d < 0.001) return t;

        t += d;

        if (t > 20.0) break; // void
    }
    return -1.0;
}

void main() {
    vec2 screen_uv = gl_FragCoord.xy / u_resolution;
    vec2 uv = (gl_FragCoord.xy * 2.0 - u_resolution) / min(u_resolution.x, u_resolution.y);

    // Look-At Camera
    float cam_speed = u_audio_time * 0.20;
    float cam_dist  = 4.3 - (u_energy * 0.4); // zoom or not
    vec3 ro = vec3(sin(cam_speed) * cam_dist, 
                   sin(u_audio_time * 0.1) * 1.3, 
                   -cos(cam_speed) * cam_dist);

    vec3 target = vec3(0.0, 0.0, 0.0);
    vec3 forward = normalize(target - ro);
    vec3 world_up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(forward, world_up));
    vec3 up = cross(right, forward); // right and forward already normalized

    // camera axes = forward + right + up

    vec3 rd = normalize(uv.x * right + uv.y * up + 1.6 * forward);

    float hit_dist = raymarch(ro, rd);

    vec3 current_frame = vec3(0.0);

    if (hit_dist > 0.0) {
        vec3 p = ro + rd * hit_dist;
        vec3 normal = calculate_normal(p);

        vec3 light_dir = normalize(vec3(1.5, 2.5, -2.0));
        float diff = clamp(dot(normal, light_dir), 0.0, 1.0);

        float fresnel = pow(1.0 - clamp(dot(normal, -rd), 0.0, 1.0), 3.0 - (u_bass_impact * 1.2));

        float color_coord = p.y * 0.35 + u_audio_time * 0.08;
        vec3 surface_color = get_scene_palette(color_coord);
        vec3 rim_tint = get_scene_palette(color_coord + 0.5);

        float rim_intensity = 1.0 + (u_treble_spark * 1.8) + (u_bass_impact * 0.6);
        current_frame = surface_color * (diff + 0.15) + rim_tint * fresnel * rim_intensity;

        // Pause freeze
        if (u_is_paused > 0.5) {
            // Procedural frost crystals
            float frost_noise = sin(p.x * 35.0) * sin(p.y * 35.0) * sin(p.z * 35.0);
            frost_noise = abs(frost_noise);

            vec3 ice_color = vec3(0.75, 0.88, 0.98);
            vec3 frost_coat = mix(vec3(0.9, 0.95, 1.0), ice_color, frost_noise);

            // Replace warm colors by matte white/cyan ice
            current_frame = mix(current_frame * 0.4, frost_coat * (diff * 0.7 + 0.3), 0.85);

            // freezed blue on edges
            current_frame += vec3(0.2, 0.5, 0.8) * fresnel * 0.6;
        }

    } else {
        // Dark background
        float bg_brightness = (u_is_paused > 0.5) ? 0.03 : (0.05 + (u_energy * 0.08));
        current_frame = get_scene_palette(uv.y * 0.2 - u_audio_time * 0.03) * bg_brightness;
    }

    // Temporal feedback loop
    vec2 trail_uv = (screen_uv - 0.5) * 0.995 + 0.5;
    vec3 past_frame = texture(u_prev_frame, trail_uv).rgb;

    vec3 final_color = current_frame;

    if (u_is_paused > 0.5) {
        final_color = current_frame;
    } else {
        float decay = 0.60 - (u_bass_impact * 0.04);
        vec3 blended_trails = past_frame * decay;
        final_color = max(current_frame, blended_trails * 0.96) + blended_trails * 0.08;
    }

    // Audion progress line (1.5% on the lower border)
    float line_y = 0.015;
    float dist_to_line = abs(screen_uv.y - line_y);

    if (dist_to_line < 0.003 && screen_uv.x <= u_progress) {
        float glow = 1.0 - (dist_to_line / 0.003);
        vec3 line_color = (u_is_paused > 0.5) ? vec3(0.5, 0.8, 1.0) : (get_scene_palette(u_progress + 0.2) * 1.5);
        final_color += line_color * glow;
    }

    frag_color = vec4(final_color, 1.0);
}