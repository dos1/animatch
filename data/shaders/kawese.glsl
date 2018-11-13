#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D al_tex;
uniform vec2 size;
uniform float kernel;
varying vec2 varying_texcoord;
varying vec4 varying_color;
varying vec4 varying_origpos;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Developed by Masaki Kawase, Bunkasha Games
// Used in DOUBLE-S.T.E.A.L. (aka Wreckless)
// From his GDC2003 Presentation: Frame Buffer Postprocessing Effects in  DOUBLE-S.T.E.A.L (Wreckless)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
vec4 KawaseBlurFilter( sampler2D tex, vec2 texCoord, vec2 pixelSize, float iteration ) {
	vec2 texCoordSample;
	vec2 halfPixelSize = pixelSize / 2.0;
	vec2 dUV = ( pixelSize.xy * vec2( iteration, iteration ) ) + halfPixelSize.xy;

	vec4 cOut;

	// Sample top left pixel
	texCoordSample.x = texCoord.x - dUV.x;
	texCoordSample.y = texCoord.y + dUV.y;

	cOut = texture2D( tex, texCoordSample );

	// Sample top right pixel
	texCoordSample.x = texCoord.x + dUV.x;
	texCoordSample.y = texCoord.y + dUV.y;

	cOut += texture2D( tex, texCoordSample );

	// Sample bottom right pixel
	texCoordSample.x = texCoord.x + dUV.x;
	texCoordSample.y = texCoord.y - dUV.y;
	cOut += texture2D( tex, texCoordSample );

	// Sample bottom left pixel
	texCoordSample.x = texCoord.x - dUV.x;
	texCoordSample.y = texCoord.y - dUV.y;

	cOut += texture2D( tex, texCoordSample );

	// Average
	cOut *= 0.25;

	return cOut;
}

void main() {
	gl_FragColor = KawaseBlurFilter(al_tex, varying_texcoord.xy, vec2(1.0) / size, kernel) * varying_color;
}
