const state={events:[],index:0,timer:null,speed:1};
const $=id=>document.getElementById(id);
const money=p=>(p/100).toFixed(2);
const pad=n=>String(n).padStart(2,'0');
function paintLevels(side,items,prior){
 const el=$(side),max=Math.max(42,...items.map(x=>x.qty));
 el.innerHTML='';
 for(let i=0;i<5;i++){
  const item=items[i],row=document.createElement('div');
  row.className=`level ${side==='asks'?'ask':'bid'}${item?'':' empty'}`;
  if(item&&(!prior||!prior.some(x=>x.price===item.price&&x.qty===item.qty)))row.classList.add('highlight');
  const price=document.createElement('span');price.className='price';price.textContent=item?money(item.price):'—';
  const track=document.createElement('div');track.className='bar-track';const bar=document.createElement('div');bar.className='bar';bar.style.width=item?`${Math.max(5,Math.round(item.qty/max*100))}%`:'0%';track.appendChild(bar);
  const qty=document.createElement('span');qty.className='qty';qty.textContent=item?item.qty:'—';
  row.append(price,track,qty);el.appendChild(row);
 }
}
function render(){
 const e=state.events[state.index-1],prev=state.events[state.index-2];
 const bids=e?e.bids:[],asks=e?e.asks:[];
 paintLevels('asks',asks,prev?.asks);paintLevels('bids',bids,prev?.bids);
 $('event-index').textContent=pad(state.index);$('counter').textContent=`${pad(state.index)} / ${pad(state.events.length)}`;$('seek').value=state.index;
 $('ask-best').textContent=asks.length?`BEST ${money(asks[0].price)}`:'NO ASKS';
 $('bid-best').textContent=bids.length?`BEST ${money(bids[0].price)}`:'NO BIDS';
 $('spread').textContent=asks.length&&bids.length?money(asks[0].price-bids[0].price):'—';
 $('event-label').textContent=e?e.label:'Step into the book.';
 const type=$('event-type');type.className='event-type '+(e?.kind==='cancel'?'cancel':e?.side==='sell'?'sell':'');type.textContent=e?e.kind==='cancel'?'CANCEL':e.side==='buy'?'BUY ORDER':'SELL ORDER':'START';
 $('event-desc').textContent=e?e.kind==='cancel'?'The resting order leaves its price level. Its place in the queue is gone.':e.trades.length?`${e.trades.length} fill${e.trades.length>1?'s':''} at resting prices. The remaining size ${e.side==='buy'?'rests as a bid':'rests as an ask'} if not fully filled.`:`The order does not cross the spread. It joins the ${e.side==='buy'?'bid':'ask'} queue at its price.`:'Press play or step forward to see orders arrive.';
 const all=e?.tape||[];$('trade-count').textContent=`${state.events.slice(0,state.index).reduce((n,x)=>n+x.trades.length,0)} FILLS`;
 $('tape').innerHTML=all.length?'':'<p class="empty">No trades yet. The first orders need a counterparty.</p>';
 for(const tr of [...all].reverse()){
  const row=document.createElement('div');row.className='tape-row';
  const desc=document.createElement('span');desc.textContent=`${tr.qty} shares @ ${money(tr.price)}`;
  const meta=document.createElement('small');meta.textContent=`Maker #${tr.maker} · Taker #${tr.taker}`;desc.appendChild(meta);
  const seq=document.createElement('b');seq.textContent=`#${pad(tr.seq)}`;row.append(desc,seq);$('tape').appendChild(row);
 }
 $('play').innerHTML=state.timer?'Pause <span>Ⅱ</span>':'Play <span>▶</span>';
}
function stop(){if(state.timer)clearInterval(state.timer);state.timer=null;$('play').innerHTML='Play <span>▶</span>'}
function step(n){state.index=Math.min(state.events.length,Math.max(0,n));render();if(state.index===state.events.length)stop()}
function start(){if(state.timer){stop();return}if(state.index===state.events.length)state.index=0;state.timer=setInterval(()=>step(state.index+1),1000/state.speed);render()}
$('back').addEventListener('click',()=>{stop();step(state.index-1)});
$('next').addEventListener('click',()=>{stop();step(state.index+1)});
$('play').addEventListener('click',start);
$('seek').addEventListener('input',e=>{stop();step(+e.target.value)});
$('speed').addEventListener('click',()=>{state.speed=state.speed===1?1.5:state.speed===1.5?2:1;$('speed').textContent=state.speed+'×';if(state.timer){stop();start()}});
fetch('./data.json').then(r=>{if(!r.ok)throw Error('Trace unavailable');return r.json()}).then(data=>{state.events=data.events;$('seek').max=data.events.length;render()}).catch(()=>{$('event-label').textContent='Trace unavailable';$('event-desc').textContent='Reload to try again.'});
