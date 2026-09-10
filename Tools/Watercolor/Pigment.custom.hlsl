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
// Lighting estimate, not a VSM visibility buffer. Reject geometry/material boundaries.
float3 src=Scene.rgb;
if (Gate<0.001 || Strength<0.001) return src;
float2 uv=GetDefaultSceneTextureUV(Parameters,14);
float2 guv=GetDefaultSceneTextureUV(Parameters,1);
float2 px=GetSceneTextureViewSize(14).zw;
float2 gpx=GetSceneTextureViewSize(1).zw;
float3 bc=max(Base.rgb,0.04);
float z=SceneTextureLookup(guv,1,false).r;
float3 normal=Normal.rgb;
float lum=max(dot(src,float3(.2126,.7152,.0722)),0.0001);
float alb=max(dot(bc,float3(.2126,.7152,.0722)),0.04);
float illum=lum/alb;
float nearBright=0,farBright=0,farDark=0,nearWeight=0,farWeight=0;
float3 p=World/max(PatchSizeCm,1.0);
float n=0.72*wash.noise(p)+0.28*wash.noise(p*2.07+float3(17.1,5.7,9.2));
float2 dirs[4]={float2(1,0),float2(-1,0),float2(0,1),float2(0,-1)};
[unroll] for(int j=0;j<8;j++){
 float radius=(j<4 ? max(RimWidthPx,0.5) : max(SoftWidthPx,1.0))*(0.8+0.4*n);
 float2 offset=dirs[j%4]*radius;
 float2 q=clamp(uv+offset*px,px,1.0-px);
 float2 g=clamp(guv+offset*gpx,gpx,1.0-gpx);
 float nz=SceneTextureLookup(g,1,false).r;
 float3 nn=SceneTextureLookup(g,8,false).rgb;
 float3 nb=SceneTextureLookup(g,5,false).rgb;
 float valid=(1-smoothstep(0.008,0.025,abs(nz-z)/max(z,1.0)))
 *smoothstep(0.90,0.99,dot(normal,nn))
 *(1-smoothstep(0.03,0.12,length(nb-bc)/max(length(bc),0.1)));
 float nl=max(dot(SceneTextureLookup(q,14,false).rgb,float3(.2126,.7152,.0722)),.0001);
 float ni=nl/max(dot(nb,float3(.2126,.7152,.0722)),.04);
 float contrast=(ni-illum)/max(ni+illum,.001);
 if(j<4){nearBright=max(nearBright,max(contrast,0)*valid);nearWeight+=valid;}
 else {farBright=max(farBright,max(contrast,0)*valid);farDark=max(farDark,max(-contrast,0)*valid);farWeight+=valid;}
}
float rim=smoothstep(.07,.28,nearBright)*saturate(nearWeight*.25);
float soft=smoothstep(.08,.35,farBright+farDark)*(1-smoothstep(.04,.20,nearBright));
float tide=(n-.5)*2.0;
float dry=RimStrength*rim*(.8+.2*n);
float bleed=BleedStrength*soft*tide;
float shadow=smoothstep(.08,.35,farBright);
float3 tint=lerp(float3(.95,.94,1.06),float3(1.04,.95,1.02),wash.noise(p*.43+11));
float3 result=src*(1.0-dry+bleed);
result*=lerp(float3(1,1,1),tint,saturate(shadow+soft*.3));
if(DebugView>.5) return lerp(src,float3(rim,soft,shadow),saturate(Gate));
return lerp(src,max(result,0),saturate(Strength*Gate));
