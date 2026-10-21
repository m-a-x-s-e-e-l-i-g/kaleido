import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {geometry,point,createMap,render} from '../site/kaleido.mjs';
for(const segments of [4,6,12])test(`${segments} segments: closed mapping, original wedge, bounds and RGBA`,()=>{
  for(const rotation of [0,17,90,179,270,359])for(const centerX of [0,500,1000]){
    const settings={segments,rotation,centerX,centerY:1000-centerX},w=72,h=48,g=geometry(w,h,settings),map=createMap(w,h,settings);
    for(let y=0;y<h;y++)for(let x=0;x<w;x++){
      const [sx,sy]=point(g,x,y);assert.ok(sx>=0&&sx<w&&sy>=0&&sy<h);assert.equal(map[y*w+x],(sy*w+sx)*4);
    }
    const source=Uint8ClampedArray.from({length:w*h*4},(_,i)=>i*37%256),before=source.slice(),out=new Uint8ClampedArray(source.length);
    render(source,out,map);assert.deepEqual(source,before);
    for(let i=0;i<map.length;i++){assert.equal(out[i*4],source[map[i]]);assert.equal(out[i*4+3],255);}
  }
});
test('four-way centered geometry has independent exact reflection oracle',()=>{
  const g=geometry(72,48,{segments:4,rotation:0});
  for(let y=0;y<48;y++)for(let x=0;x<72;x++)assert.deepEqual(point(g,x,y),[Math.max(x,71-x),Math.max(y,47-y)]);
});
test('reject invalid geometry and aliased buffers',()=>{
  for(const settings of [{segments:8},{rotation:360},{rotation:-1},{centerX:1001},{centerY:-1}])assert.throws(()=>geometry(72,48,settings),RangeError);
  const buffer=new Uint8ClampedArray(16);assert.throws(()=>render(buffer,buffer,new Uint32Array(4)),RangeError);
});
test('browser coordinates match the compiled C Bayer mapper fixtures',()=>{
  const fixtures=JSON.parse(readFileSync(new URL('../evidence/geometry.json',import.meta.url),'utf8'));
  for(const fixture of fixtures){
    const g=geometry(72,48,fixture);
    for(const [x,y,sx,sy] of fixture.points)assert.deepEqual(point(g,x,y),[sx,sy]);
  }
});
