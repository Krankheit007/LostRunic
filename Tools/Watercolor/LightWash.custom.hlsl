struct LRWashNoise
{
 float hash(float3 p) { p=frac(p*0.1031); p+=dot(p,p.yzx+33.33); return frac((p.x+p.y)*p.z); }
 float noise(float3 p) {
 float3 i=floor(p),f=frac(p); f=f*f*(3.0-2.0*f);
 return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
 lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),lerp(hash(i+float3(0,1,1)),hash(i+float3(1,1,1)),f.x),f.y),f.z);
 }
};
LRWashNoise wash;
float3 p=World/max(PatchSizeCm,1.0);
float n=0.72*wash.noise(p)+0.28*wash.noise(p*2.07+float3(17.1,5.7,9.2));
float pooled=smoothstep(0.28,0.72,n);
return 1.0-saturate(Strength)*(1.0-pooled);
