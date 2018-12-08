#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D al_tex;
uniform sampler2D tex_bg;
varying vec2 varying_texcoord;
varying vec4 varying_color;
varying vec4 varying_pos;

void main() {
	vec4 board = texture2D(al_tex, varying_texcoord);
	vec4 bg = texture2D(tex_bg, varying_pos.xy / 2.0 + vec2(0.5, 0.5));
	vec4 combined = mix(bg, board, varying_color.a);
	gl_FragColor = combined;
}
