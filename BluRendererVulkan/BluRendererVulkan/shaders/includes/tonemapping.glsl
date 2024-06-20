#ifndef TONEMAP
    #define TONEMAP 1
#endif
#ifndef TONEMAP_EXPOSURE
	#define TONEMAP_EXPOSURE 4.5
#endif
#ifndef TONEMAP_GAMMA
	#define TONEMAP_GAMMA 2.2 
#endif

//Fast Filmic Tonemapping
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15; //Shoulder Strength
	float B = 0.50; //Linear Strength
	float C = 0.10; //Linear Angle
	float D = 0.20; //Toe Strength
	float E = 0.02; //Toe Numerator
	float F = 0.30; //Toe Denominator
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ApplyTonemap(vec3 rbg) {
	// Tone mapping
	//vec3 color = Uncharted2Tonemap(rbg * TONEMAP_EXPOSURE);
	//color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
	// Gamma correction
	vec3 color = pow(rbg, vec3(1.0f / TONEMAP_GAMMA));
	return color;
}