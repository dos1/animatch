#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D al_tex;
uniform sampler2D tex_bg;
varying vec2 varying_texcoord;
varying vec4 varying_color;

void main() {
	vec4 board = texture2D(al_tex, varying_texcoord);
	vec4 bg = texture2D(tex_bg, varying_texcoord);
	vec4 combined = mix(bg, board, board.a);
	combined /= combined.a;
	combined.a = (board.a > 0.5) ? 1.0 : board.a;
	combined *= combined.a;
	gl_FragColor = combined * varying_color;
}
