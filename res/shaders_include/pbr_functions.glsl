#include "shader_base.glsl"
#include "utils.glsl"

const int MAX_LOD_LEVEL = 11;

struct DataMaterial {
	vec3 albedo;
	float opacity;

	float occlusion;
	float roughness;
	float metalness;
	// [float] unused
};

vec3 BRDFDirect(vec3 L, vec3 N, vec3 V, DataMaterial mat);
vec3 BRDFIndirect(vec3 L, vec3 N, vec3 V, DataMaterial mat, sampler2D tex_environment);

vec3 GetAlbedo(DataMaterial data);										// Get the surface albedo
vec3 GetReflectance(DataMaterial data);									// Get the surface reflectance
vec3 SampleEnvironment(vec3 direction, float lodLevel, sampler2D tex_environment);					// Sample the EnvironmentTexture cubemap
float DistributionGGX(vec3 N, vec3 H, float roughness);					// GGX equation for distribution function
float GeometrySmith(vec3 N, vec3 inDir, vec3 outDir, float roughness);	// Geometry term in both directions
vec3 FresnelSchlick(vec3 f0, vec3 V, vec3 H);							// Schlick simplification of the Fresnel term



vec3 BRDFDirect(vec3 L, vec3 N, vec3 V, DataMaterial mat) {
	vec3 H = normalize(L + V);
	
	// compute the diffuse term
	vec3 diffuse = GetAlbedo(mat) * PI_INV;
	
	// compute the specular term
	float D = DistributionGGX(N, H, mat.roughness);
	float G = GeometrySmith(N, L, V, mat.roughness);
	
	float cosI = clamped_dot(N, L);
	float cosO = clamped_dot(N, V);
	
	vec3 specular = clamp01(vec3((D * G) / (4.0 * cosO * cosI + 0.00001)));
	
	// combine diffuse and specular, mantaining energy conservation
	// Compute the Fresnel term between the half direction and the view direction
	vec3 fresnel = FresnelSchlick(GetReflectance(mat), V, H);

	// Move the incidence factor to affect the combined light value
	float incidence = clamped_dot(N, L);
	
	// Linearly interpolate between diffuse and specular, using the fresnel value
	vec3 lighting = mix(diffuse, specular, fresnel) * incidence;
	
	return lighting;
}

vec3 BRDFIndirect(vec3 L, vec3 N, vec3 V, DataMaterial mat, sampler2D tex_environment) {
	// compute the indirect diffuse term
//	vec3 diffuse = SampleEnvironment(N, 1.0, tex_environment) * mat.albedo;
	vec3 diffuse = SampleEnvironment(N, 1.0, tex_environment) * GetAlbedo(mat);
	
	// compute the indirect specular term
	// Compute the reflection vector with the viewDir and the normal
	vec3 reflectionDir = reflect(-V, N);
	
	// Sample the environment map using the reflection vector,
	// at a specific LOD level
	float lodLevel = pow(mat.roughness, 0.25);
	
	vec3 specular = clamp01(
		SampleEnvironment(reflectionDir, lodLevel, tex_environment)
		* GeometrySmith(N, reflectionDir, V, mat.roughness)
	);
	
	// combine diffuse and specular, mantaining energy conservation
	// Compute the Fresnel term between the normal and the view direction
	vec3 fresnel = FresnelSchlick(GetReflectance(mat), V, N);

	return specular;
	// Linearly interpolate between the diffuse and specular term
	return mix(diffuse, specular, fresnel);
}

// Get the surface albedo
vec3 GetAlbedo(DataMaterial data) {
	return mix(data.albedo, vec3(0.0), data.metalness);
}

// Get the surface reflectance
vec3 GetReflectance(DataMaterial data)
{
	// We use a fixed value for dielectric, with a typical value of 4%
	return mix(vec3(0.04), data.albedo, data.metalness);
}

// Sample the EnvironmentTexture cubemap
// lodLevel: between 0 and 1 to select from the highest to the lowest mipmap
vec3 SampleEnvironment(vec3 direction, float lodLevel, sampler2D tex_environment)
{
	// Flip the Z direction, because the cubemap is left-handed
	direction.z *= -1;

	// // cubemap
	// return texture(tex_environment, direction).rgb;
	vec2 uv = uv_spherical_mapping(direction);

//	return texture(tex_environment, uv).rgb;
	return textureLod(tex_environment, uv, lodLevel * MAX_LOD_LEVEL).rgb;
}

// Geometry term in one direction, for GGX equation
float GeometrySchlickGGX(float cosAngle, float roughness)
{
	float roughness2 = roughness * roughness;

	return             (2 * cosAngle) /
		(cosAngle + sqrt(roughness2 + (1 - roughness2) * cosAngle * cosAngle));
//	float nom   = cosAngle;
//    float denom = cosAngle * (1.0 - roughness) + roughness;
//	
//    return nom / denom;
}

// GGX equation for distribution function
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float roughness2 = roughness * roughness;

	float dotNH = clamped_dot(N, H);

	float expr = dotNH * dotNH * (roughness2 - 1.0) + 1.0;

	return roughness2 / (PI * expr * expr);
}

// Geometry term in both directions, following Smith simplification.
// Divides it in the product of both directions
float GeometrySmith(vec3 N, vec3 inDir, vec3 outDir, float roughness)
{
	// Occlusion in input direction (shadowing)
	float ggxIn = GeometrySchlickGGX(clamped_dot(N, inDir), roughness);

	// Occlusion in output direction (masking)
	float ggxOut = GeometrySchlickGGX(clamped_dot(N, outDir), roughness);

	// Total occlusion is a product of both
	return ggxIn * ggxOut;
}

// Schlick simplification of the Fresnel term
vec3 FresnelSchlick(vec3 f0, vec3 V, vec3 H)
{	
	return f0 + (vec3(1.0) - f0) * pow(1.0 - clamped_dot(V, H), 5.0);
}
