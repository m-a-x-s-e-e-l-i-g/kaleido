import {createMap,render} from './kaleido.mjs';
// Keep the full Mirror Worlds source resolution in the preview and PNG export.
const $=id=>document.getElementById(id),canvas=$('preview'),ctx=canvas.getContext('2d'),{width,height}=canvas;
let settings={segments:6,rotation:0,centerX:500,centerY:500},source,output,map,queued=false,original=false,uploadGeneration=0;
function update(){
  $('rotation-value').value=`${settings.rotation}°`;
  $('center-x-value').value=`${settings.centerX/10}%`;$('center-y-value').value=`${settings.centerY/10}%`;
  document.querySelectorAll('[data-segments]').forEach(b=>b.setAttribute('aria-pressed',Number(b.dataset.segments)===settings.segments));
  $('compare').setAttribute('aria-pressed',original);$('compare').textContent=original?'Show kaleidoscope ↔':'Show original ↔';
  $('caption').textContent=original?'The original · before the turn':`${settings.segments} pieces · ${settings.rotation}° · center ${settings.centerX/10}% / ${settings.centerY/10}%`;
  canvas.setAttribute('aria-label',original?'Original source image':`${settings.segments}-segment kaleidoscope at ${settings.rotation} degrees`);
  if(!source)return;
  if(original){ctx.putImageData(source,0,0);return;}
  map=createMap(width,height,settings);render(source.data,output.data,map);ctx.putImageData(output,0,0);
}
function schedule(){if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;update();});}
function useImage(image){
  const scratch=document.createElement('canvas');scratch.width=width;scratch.height=height;const c=scratch.getContext('2d');
  const iw=image.naturalWidth||image.width,ih=image.naturalHeight||image.height,scale=Math.max(width/iw,height/ih);
  c.drawImage(image,(width-iw*scale)/2,(height-ih*scale)/2,iw*scale,ih*scale);
  source=c.getImageData(0,0,width,height);output=ctx.createImageData(width,height);update();canvas.hidden=false;$('fallback').hidden=true;$('compare').disabled=false;$('save').disabled=false;
}
document.querySelectorAll('[data-segments]').forEach(b=>b.addEventListener('click',()=>{settings.segments=Number(b.dataset.segments);schedule();}));
for(const [id,key] of [['rotation','rotation'],['center-x','centerX'],['center-y','centerY']])$(id).addEventListener('input',e=>{settings[key]=Number(e.target.value);schedule();});
$('compare').addEventListener('click',()=>{original=!original;schedule();});
$('reset').addEventListener('click',()=>{settings={segments:6,rotation:0,centerX:500,centerY:500};original=false;for(const id of ['rotation','center-x','center-y'])$(id).value=id==='rotation'?0:500;schedule();});
$('upload').addEventListener('change',async e=>{
  const file=e.target.files[0];if(!file)return;const generation=++uploadGeneration;
  if(!file.type.startsWith('image/')){$('message').textContent='Choose an image file, such as a JPEG or PNG.';return;}
  if(file.size>30*1024*1024){$('message').textContent='Choose an image smaller than 30 MB.';return;}
  const url=URL.createObjectURL(file),image=new Image();
  try{image.src=url;await image.decode();if(generation!==uploadGeneration)return;useImage(image);$('message').textContent=`${file.name} · stays in your browser.`;}
  catch{if(generation===uploadGeneration)$('message').textContent='This image could not be opened. Try a JPEG, PNG or WebP.';}
  finally{URL.revokeObjectURL(url);}
});
$('save').addEventListener('click',()=>{
  // Flush pending sliders before exporting the visible composition.
  update();canvas.toBlob(blob=>{if(!blob){$('message').textContent='The image could not be saved. Please try again.';return;}
    const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=`kaleido-${original?'original':settings.segments+'-pieces'}.png`;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
  },'image/png');
});
const initialGeneration=uploadGeneration,image=new Image();image.src='assets/seaside-scene.webp';
image.decode().then(()=>{if(uploadGeneration===initialGeneration)useImage(image);}).catch(()=>{$('message').textContent='The sample could not load. Try your own photo.';$('caption').textContent='Choose a photo to begin.';});
