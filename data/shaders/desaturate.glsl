#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D al_tex;
uniform bool enabled;
varying vec2 varying_texcoord;
varying vec4 varying_color;

void main() {
	vec4 color = texture2D(al_tex, varying_texcoord);
	vec3 desaturated = vec3(dot(color.rgb / color.a, vec3(0.22, 0.707, 0.071)));
	desaturated *= color.a;
	gl_FragColor = (enabled ? vec4(mix(desaturated, color.rgb, 0.333), color.a) : color) * varying_color;
}
