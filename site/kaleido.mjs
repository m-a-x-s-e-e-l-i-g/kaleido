// SPDX-License-Identifier: GPL-2.0-or-later
// Same Q14 sector convention as the camera; RGB illustration, not a RAW emulator.
const sine = Array.from({length:91}, (_, i) => Math.round(Math.sin(i*Math.PI/180)*16384));
function sin(n) { n=((n%360)+360)%360; const sign=n>=180?-1:1; if(n>=180)n-=180;if(n>90)n=180-n;return sign*sine[n]; }
const cos = n => sin(n+90);
const rounded = n => n>=0?Math.floor((n+16384)/32768):-Math.floor((-n+16384)/32768);
const edge = (v,n) => {if(n<=1)return 0;const period=2*(n-1);v=((v%period)+period)%period;return v<n?v:period-v;};
export function geometry(width,height,{segments=6,rotation=0,centerX=500,centerY=500}={}) {
  if(!Number.isInteger(width)||width<2||width>8192||!Number.isInteger(height)||height<2||height>8192||
    ![4,6,12].includes(segments)||![rotation,centerX,centerY].every(Number.isInteger)||
    rotation<0||rotation>=360||centerX<0||centerX>1000||centerY<0||centerY>1000) throw new RangeError('Invalid kaleidoscope geometry');
  const cx=Math.floor(((width-1)*centerX*2+500)/1000),cy=Math.floor(((height-1)*centerY*2+500)/1000),step=360/segments;
  const sectors=Array.from({length:segments},(_,k)=>{
    const angle=rotation+k*step;
    const t=k%2?2*rotation+(k+1)*step:-k*step,c=cos(t),s=sin(t);
    return {x:cos(angle),y:sin(angle),a:c,b:k%2?s:-s,c:s,d:k%2?-c:c};
  });
  return {width,height,cx,cy,sectors};
}
export function point(g,x,y) {
  const dx=x*2-g.cx,dy=y*2-g.cy;let k=0;
  for(let i=0;i<g.sectors.length;i++){
    const s=g.sectors[i],n=g.sectors[(i+1)%g.sectors.length];
    if(s.x*dy-s.y*dx>=0&&n.x*dy-n.y*dx<0){k=i;break;}
  }
  const s=g.sectors[k];
  return [edge(rounded(g.cx*16384+s.a*dx+s.b*dy),g.width),edge(rounded(g.cy*16384+s.c*dx+s.d*dy),g.height)];
}
export function createMap(width,height,settings) {
  const g=geometry(width,height,settings),map=new Uint32Array(width*height);
  for(let y=0;y<height;y++)for(let x=0;x<width;x++){
    const [sx,sy]=point(g,x,y);map[y*width+x]=(sy*width+sx)*4;
  }
  return map;
}
export function render(source,destination,map) {
  if(source.length!==destination.length||source.length!==map.length*4||source===destination)throw new RangeError('Separate, equally sized RGBA buffers required');
  for(let i=0;i<map.length;i++){
    const from=map[i],to=i*4;destination[to]=source[from];destination[to+1]=source[from+1];destination[to+2]=source[from+2];destination[to+3]=255;
  }
}
