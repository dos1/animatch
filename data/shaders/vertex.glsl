#ifdef GL_ES
precision mediump float;
#endif

attribute vec4 al_pos;
attribute vec4 al_color;
attribute vec2 al_texcoord;
attribute vec2 al_user_attr_0;
uniform mat4 al_projview_matrix;
uniform bool al_use_tex_matrix;
uniform mat4 al_tex_matrix;
varying vec4 varying_color;
varying vec2 varying_texcoord;
varying vec2 varying_origtex;
varying vec4 varying_origpos;
varying vec4 varying_pos;

void main() {
	varying_color = al_color;
	varying_origpos = al_pos;
	if (al_use_tex_matrix) {
		vec4 uv = al_tex_matrix * vec4(al_texcoord, 0, 1);
		varying_texcoord = uv.xy;
	} else {
		varying_texcoord = al_texcoord;
	}

	varying_origtex = vec2(al_user_attr_0.x, 1.0 - al_user_attr_0.y);
	varying_pos = al_projview_matrix * al_pos;
	gl_Position = varying_pos;
}
