/*sp 2025 8 19
partial decodable compression by static PPM.
it based on sp v0.2(C)2004 11 28 Daisuke Okanohara*/
const RICE_MASK=3,R_SHIFT=14,MAXBUF=1<<20,MAXDEPTH=10;
function rightbits(len,c){return(1<<len)-1&c}
class riceEncode{
constructor(fp){
	this.outfp=fp;this.c=32;this.b=0;this.p=fp.length
}
putbits(c,len){
	for(;len>=this.c;this.c=32)
		len-=this.c,
		this.b|=rightbits(this.c,c>>>len),
		this.outfp[this.p++]=this.b&255,
		this.outfp[this.p++]=this.b>>8&255,
		this.outfp[this.p++]=this.b>>16&255,
		this.outfp[this.p++]=this.b>>>24,
		this.b=0;
	this.b|=rightbits(len,c)<<(this.c-=len);
}
putbit(bit){
	this.c--;
	if(bit)this.b|=(1<<this.c);
	if(!this.c)
		this.outfp[this.p++]=this.b&255,
		this.outfp[this.p++]=this.b>>8&255,
		this.outfp[this.p++]=this.b>>16&255,
		this.outfp[this.p++]=this.b>>>24,
		this.b=0,this.c=32
}
code(n,mask){
	this.putbit(n<0);
	if(n<0)n=~n;
	for(var b=0;n>>++b;);
	this.unsignedcode(b-1,mask);
	b>1?this.putbits(n,b-1):this.putbit(n>0);
}
unsignedcode(n,mask){
	this.putbits(0,n>>>mask);this.putbit(1);this.putbits(n,mask)
}
flush(){this.putbits(0,31)}
}
class riceDecode{
constructor(fp){this.infp=fp;this.p=fp.p>>>0;this.c=this.b=0}
getbit(){
	if(--this.c>=0)return this.b>>>this.c&1;
	this.c=31;
	this.b=this.infp[this.p++]|this.infp[this.p++]<<8|this.infp[this.p++]<<16|this.infp[this.p++]<<24;
	return this.b>>>31&1;
}
getbits(n,x){
	for(;n>this.c;this.c=32)
		n-=this.c,
		x|=rightbits(this.c,this.b)<<n,
		this.b=this.infp[this.p++]|this.infp[this.p++]<<8|this.infp[this.p++]<<16|this.infp[this.p++]<<24;
	return x|rightbits(n,this.b>>>(this.c-=n))
}
decode(mask){
	let m=this.getbit(),n=this.unsigneddecode(mask);
	n=n?1<<n|this.getbits(n):this.getbit();
	return m?~n:n
}
unsigneddecode(mask){
	for(var n=0;!this.getbit();)n++;
	return(n<<mask)+this.getbits(mask)
}}
//range coder
class rangeEncoder{
constructor(outfp,o){this.o=o||outfp.length;this.setup(this.outfp=outfp)}
pc(c){this.outfp[this.o++]=c}
setup(){this.low=this.count=0;this.range=-1>>>0}
flush(i){for(i=4;i--;this.low<<=8)this.pc(this.low>>>24)}
//---:normal coding----------
encode(cumFreq,freq,totFreq){
	let r=this.range/totFreq>>>0,l=this.low+r*cumFreq;
	if(!r)throw"range error";
	for(r*=freq;!((l^l+r)>>24)||r<65536&&(r=-l&65535);r*=256)this.pc(l>>>24),l<<=8;
	this.range=r;this.low=l
}
//---:using shift instead of dividing----------
encodeshift(cumFreq,freq,totShift){
	let r=this.range>>>totShift,l=this.low+r*cumFreq;
	if(!r)throw"range error";
	for(r*=freq;!((l^l+r)>>24)||r<65536&&(r=-l&65535);r*=256)this.pc(l>>>24),l<<=8;
	this.range=r;this.low=l
}
//---:coding number lower than maxnumber----------
encodeNumber(number,maxnumber){this.encode(number,1,maxnumber)}
}
class rangeDecoder{
setup(In,p){this.infp=In;this.p=In.p>>>0;this.init()}
init(){
	this.range=-1>>>0;this.low=0;
	for(let i=4;i--;)this.code=this.code<<8|this.infp[this.p++];
}
//---:normalize range and low-----------
normalize(){
	for(;!((this.low^this.low+this.range)>>24)||this.range<65536&&(this.range=-this.low&65535);this.range*=256)
		this.code=this.code<<8|this.infp[this.p++],this.low<<=8;
}
//---:get next accumurate prob using shift----------
getfreqshift(totshift){
	let t=(this.code-this.low>>>0)/(this.range>>>=totshift)>>>0;
	if(t>=1<<totshift)throw"totfreq error";
	return t
}
//---:get next accumurate prob----------
getfreq(totFreq){
	let t=(this.code-this.low>>>0)/(this.range=this.range/totFreq>>>0)>>>0;
	if(t>=totFreq)throw"totfreq error";
	return t
}
basic_getCharacter(freq,cumFreq,n){
	let tmp=(this.code-this.low>>>0)/this.range>>>0,i=0,j=n,k;
	if(tmp>=cumFreq[n])throw"basic_getCharacter error";
	for(;i<j;cumFreq[k+1]>tmp?j=k:i=k+1)k=i+j>>1;
	let cf=cumFreq[i],fr=cumFreq[i+1]-cf;
	this.low+=cf*this.range;
	this.range*=fr;
	this.normalize();
	return i
}
basic_getCharacter2(cumFreq,n){
	let tmp=(this.code-this.low>>>0)/this.range>>>0,i=0,j=n,k;
	if(tmp>=cumFreq[n])throw"basic_getCharacter error";
	for(;i<j;cumFreq[k+1]>tmp?j=k:i=k+1)k=i+j>>1;
	let cf=cumFreq[i],fr=cumFreq[i+1]-cf;
	this.low+=cf*this.range;
	this.range*=fr;
	this.normalize();
	return i
}
getCharacterShift2(cumFreq,n,totShift){
	this.range>>>=totShift;
	return this.basic_getCharacter2(cumFreq,n);
}
getCharacter(freq,cumFreq,n){
	this.range=this.range/cumFreq[n]>>>0;
	return this.basic_getCharacter(freq,cumFreq,n)
}
getCharacterShift(freq,cumFreq,n,totShift){
	this.range>>>=totShift;
	return this.basic_getCharacter(freq,cumFreq,n);
}
decodeNumber(maxNumber){
	let tmp=(this.code-this.low)/(this.range=this.range/maxNumber)>>>0;
	this.low+=tmp*this.range;
	this.normalize();
	return tmp
}
decode(cumfreq,freq){
	this.low+=cumfreq*this.range;
	this.range*=freq;
	this.normalize()
}}
//range coder util
function normalize(freq,shift){
	let freqMax=1<<shift,total=0,l=freq.length,i=l;
	for(;i;)total+=freq[--i];
	if(!total)return 0;
	let total2=0,t;
	for(i=l;i;)
		if(t=freq[--i])
			t*=freqMax,total2+=freq[i]=t<total?1:t/total>>>0;
	if(total2>freqMax){
		//Decreasing each frequency
		for(t=total2-freqMax;t;)
			for(i=0;i<l;i++)
				if(freq[i]>1){
					--freq[i];
					if(!--t)break
				}
	}else if(total2<freqMax)
		//Increasing each frequency
		for(t=freqMax-total2;t;)
			for(i=0;i<l;i++)
				if(freq[i]){
					++freq[i];
					if(!--t)break
				}
	return total
}
function normalize2(freq,shift,size){
	let freqMax=1<<shift,total=0,i=size;
	for(;i;)total+=freq[--i];
	if(!total)return 0;
	let total2=0,kind=0,t;
	for(;i<size;kind++)t=freq[i]*freqMax,total2+=freq[i++]=t<total?1:t/total>>>0;
	if(total2>freqMax){
		for(t=total2-freqMax;t;)
			for(i=0;i<size;i++)
				if(freq[i]>1){--freq[i];if(!--t)break}
	}else if(total2<freqMax)
		for(t=freqMax-total2;t;)
			for(i=0;i<size;i++)
				if(freq[i]){++freq[i];if(!--t)break}
	return total
}
function makeCumFreq_dec(cumFreq,shift){
	let freqMax=1<<shift,total=0,l=cumFreq.length,i=l;
	for(cumFreq[l-1]=0;i;)total+=cumFreq[--i];
	if(!total)return;
	let total2=0,kind=0,t;
	for(i=l--;i;)if(t=cumFreq[--i])
		t*=freqMax,kind++,
		total2+=cumFreq[i]=t<total?1:t/total>>>0;
	if(total2>freqMax){
		for(t=total2-freqMax;t;)
			for(i=0;i<l;i++)
				if(cumFreq[i]>1){
					--cumFreq[i];
					if(!--t)break
				}
	}else if(total2<freqMax)
		for(t=freqMax-total2;t;)
			for(i=0;i<l;i++)
				if(cumFreq[i]){
					++cumFreq[i];
					if(!--t)break
				}
	for(i=t=0,l++;i<l;t+=kind)kind=cumFreq[i],cumFreq[i++]=t
}
function makeCumFreq_dec1(freq,cumFreq){
	for(let i=0,c=cumFreq[0],size=freq.length;i<size;)cumFreq[i+1]=c+=freq[i++]
}
function makeCumFreq_dec2(freq,cumFreq,size){
	for(let i=cumFreq[0]=0,c=0;i<size;)cumFreq[i+1]=c+=freq[i++]
}
function makeCumFreq(freq,cumFreq){
	for(let i=0,c=cumFreq[0],size=freq.length;++i<size;)cumFreq[i]=c+=freq[i-1]
}
function makeCumFreq2(freq,cumFreq,size){
	for(let i=cumFreq[0]=0,c=0;++i<size;)cumFreq[i]=c+=freq[i-1]
}
//spNode
class node{
constructor(i_freq,d,par,c){
	this.totalFreq=this.oriTotalFreq=this.toBit=this.kl_div=0,
	this.used=this.mearged=this.initialized=false,
	this.meargedNumber=-1,this.depth=d,this.parent=par,this.ch=c;
	let i=256,min=-1>>>1,size=0,f;
	for(this.children=[];i;)if(f=i_freq[--i]){
		size++;
		this.oriTotalFreq+=f;
		if(f<min)min=f
	}
	this.nc=new Uint8Array(size);
	this.freq=new Uint32Array(size);
	for(i=size=0;i<256;i++)if(f=i_freq[i]){
		this.nc[size]=i;
		if(f>2){
			let k=0;
			for(;f>>++k;);
			f>>=k-=k>>1;
			i_freq[i]=f<<=k
		}
		this.totalFreq+=this.freq[size++]=f
	}
	this.size=size
}
kl_divergence(par){
	let{freq,nc,totalFreq}=this,kl=0,pf=par.freq,pn=par.nc,pt=par.totalFreq;
	for(let i=0,j=0,l=freq.length,m=pf.length,c;i<l;kl+=c*Math.log(c/(pf[j]/pt))){
		//search parent
		for(c=nc[i];pn[j]!==c&&j<m;)j++;
		if(j===m)throw"kl_dirvergence error";
		c=freq[i++]/totalFreq
	}
	return(kl/Math.LN2)*this.oriTotalFreq
}
kl_divergence2(par){
	let{freq,nc,totalFreq}=this,kl=0,pf=par.freq,pn=par.cn,pt=par.totalFreq;
	for(let i=0,j=0,l=freq.length,m=pf.length,c;i<l;kl+=c*Math.log(c/(pf[j]/pt))){
		//search parent
		for(c=nc[i];pn[j]!==c&&j<m;)j++;
		if(j===m)throw"kl_dirvergence error";
		c=freq[i++]/totalFreq
	}
	return(kl/Math.LN2)*this.oriTotalFreq
}
riceUnsignedN(n,mask){return(n>>mask)+mask+1}
checkBitN(parent){
	let{freq,nc}=this,i=0,n0=nc.length,bitcount=this.riceUnsignedN(n0,RICE_MASK),count=0,dif=0,pn=parent.nc,total=pn.length,ltotal=total&&Math.log(total),ln0=n0&&Math.log(n0),ln1=total>n0&&Math.log(total-n0);
	bitcount+=(total*ltotal-n0*ln0-(total-n0)*ln1)/Math.LN2>>>0;
	if(n0>1)for(;i<total&&count<n0;)
		if(pn[i++]===nc[count]){
			let t=freq[count]-1,k=0;
			for(;t;k++)t>>=1;
			bitcount+=this.riceUnsignedN(k,2);
			if(k)bitcount+=k>>1;
			count++
		}
	return++bitcount
}
checkBitN3(){
	let bitcount=0,p=0,t=this.totalFreq,i=this.nc.length,freq=this.freq;
	for(;t;p++)t>>=1;p>>=1;
	if(i^1)bitcount+=this.riceUnsignedN(p,RICE_MASK);
	for(;i;)bitcount+=this.riceUnsignedN(freq[--i]-1,p);
	return bitcount
}
checkBitN2(){
	let n0=this.children.length,ln0=n0&&Math.log(n0),ln1=256>n0&&Math.log(256-n0),b=this.riceUnsignedN(n0,RICE_MASK);
	return b+=(1419.565425786768-n0*ln0-(256-n0)*ln1)/Math.LN2>>>0;
}
getSize(){
	let i,n=1;
	for(i of this.children)n+=i.getSize();
	return n
}
free(){
	for(let i of this.children)i.free();delete this.children
}
getNonMeargeSize(){
	let i,n=this.mearged&1;
	for(i of this.children)n+=i.getNonMeargeSize();
	return n
}
getUsedSize(){
	let i,n=this.used&1;
	for(i of this.children)n+=i.getUsedSize();
	return n
}
searchPath(buf,p,historyLimit){
	if(p>=historyLimit)//liner search
		for(let cur=buf[p],S=this.children,left=0,right=S.length,center;left<right;S[center].ch<cur?left=center+1:right=center)
			if(S[center=left+right>>1].ch===cur)return S[center].searchPath(buf,p-1,historyLimit);
	//p<0 or not found
	return this
}
outputRice(rc,m){
	let{nc,oriFreq,children}=this,n=nc.length,l=this.used;
	rc.unsignedcode(n-1,RICE_MASK);
	rc.putbit(l);
	if(l&&n>1)for(let i=0;i<n;i++){
		let t=oriFreq[i],k=0;
		for(;t>>++k;);
		rc.unsignedcode(k-1,2);
		if(k>2)l=k>>1,rc.putbits(t>>l+(k&1),l)
	}
	if(m)return this.meargedNumber=m_number++;
	n=children.length;rc.putbit(n>0);
	if(n){
		rc.unsignedcode(n-1,RICE_MASK);//about 10bit
		for(let i=0;i<n;)children[i++].outputRice(rc);
	}
}
outputRice4(dad,ra,m){
	let{nc,children}=this,pn=dad.nc,n=nc.length,sum=pn.length,i=0,c=0;
	for(;c<n;)
		if(pn[i++]===nc[c])ra.encode(0,n,sum),c++;
		else ra.encode(n,sum-n,sum);
	if(m)return this.meargedNumber=m_number++;
	if(n=children.length)
		for(i=c=0;i<256&&c<n;)
			if(children[c].ch===i++)
				ra.encodeshift(0,n,8),
				children[c++].outputRice4(this,ra);
			else ra.encodeshift(n,256-n,8)
}
checkLog2(n){
	if(n<0)throw"error";
	for(var t=0;n;t++)n>>>=1;
	return t
}
mearge(){
	let c=[],cs=0,i;
	for(i of this.children)i.children.length?i.mearge():c[cs++]=i;
	//greedy stratedy
	let cf=new Uint32Array(256),new_c=[];
	for(;cs>1;){
		let mn=i=0;
		for(;i<cs;i++)
			for(let j=i;++j<cs;){
				let a=c[i],b=c[j];
				while(a.mearged)a=a.meargedNode;
				while(b.mearged)b=b.meargedNode;
				if(a===b)continue;
				cf.fill(0);
				let F=a.freq,N=a.nc,k=F.length,l=k;
				for(;k;)cf[N[--k]]+=F[k];
				F=b.freq,N=b.nc,l+=k=F.length;
				for(;k;)cf[N[--k]]+=F[k];
				let t=new node(cf,a.depth,a.parent,a.ch),gain=a.kl_divergence2(t)+b.kl_divergence2(t);
				t.toBit=t.checkBitN3();
				let bitCount=a.toBit+b.toBit-t.toBit+l-t.freq.length+1;
				//check the increasing cost and benefit
				if(gain<bitCount){
					//mearge
					a.mearged=b.mearged=true;
					a.meargedNode=b.meargedNode=t;
					t.toBit=Math.min(a.toBit,b.toBit);
					new_c.push(t);mn++
				}
			}
		for(i=0;i<cs;i++)c[i].mearged||new_c.push(c[i]);
		i=c;c=new_c;new_c=i;i.length=0;cs=c.length;
		if(!mn)break
	}
}
set_ncSkip2(){
	if(this.initialized)return;
	let{freq,children,ch,nc,depth,parent}=this,i=nc.length,cs=children.length,ncSkip=this.ncSkip=[];
	this.initialized=1;
	this.oriFreq=freq.slice();
	normalize(freq,R_SHIFT);
	makeCumFreq(freq,this.cumFreq=new Uint32Array(i));
	if(depth)
		a:for(let pn=parent.nc,ps=parent.ncSkip,p=pn.length,t,c;i;ncSkip[i]=t){
			for(t=this,c=nc[--i];p;)if(pn[--p]===c){
				t=ps[p];
				if(t.depth===depth)//find the children whose has ch
					for(let S=t.children,l=0,r=S.length,m;l<r;c<ch?l=m+1:r=m)
						if((c=S[m=l+r>>1].ch)===ch){
							t=S[m];continue a
						}
				break
			}
	}else for(;i;){
		ncSkip[--i]=this;//既定値=自身
		//対応する子nodeを探す
		for(let c=nc[i],l=0,r=cs,m;l<r;children[m].ch<c?l=m+1:r=m)
			if(children[m=l+r>>1].ch===c){
				ncSkip[i]=children[m];break
			}
	}
	for(;cs;)children[--cs].set_ncSkip2()
}}
//spDNode
class dNode{
constructor(){}
inputRice(rc,d){
	let size=rc.unsigneddecode(RICE_MASK)+1,S=this.children=[];
	this.depth=d;
	this.No=totalNumber++;
	this.nc=new Uint8Array(this.size=size);
	if(rc.getbit()){
		let cf=this.cumFreq=new Uint32Array(size+1);
		if(size==1)cf[0]=1;
		else for(let i=0,k,b;i<size;cf[i++]=k>2?rc.getbits(b=k>>1)<<b+(k&1):k)
			k=rc.unsigneddecode(2)+1;
		makeCumFreq_dec(cf,R_SHIFT)
	}
	if(rc.getbit())
		for(let i=0,c=rc.unsigneddecode(RICE_MASK)+1;i<c;)
			(S[i++]=new dNode).inputRice(rc,d+1)
}
inputRice4(parent,rd){
	this.parent=parent;
	let{nc,children}=this,i=0,count=0,pn=parent.nc,total=pn.length,n=nc.length,freq=new Uint32Array([n,total-n]),cumFreq=new Uint32Array(3);
	makeCumFreq_dec1(freq,cumFreq);
	for(;count<n;i++)
		if(!rd.getCharacter(freq,cumFreq,2))nc[count++]=pn[i];
	if(n=children.length){
		freq[0]=n;freq[1]=256-n;
		makeCumFreq_dec1(freq,cumFreq);
		for(i=count=0;i<256&&count<n;i++)
			if(!rd.getCharacterShift(freq,cumFreq,2,8))
				children[count].ch=i,
				children[count++].inputRice4(this,rd)
	}
}
searchPath(buf,p,historyLimit){
	if(p>=Math.max(historyLimit,0)){
		//binary search
		let S=this.children,b=buf[p],left=0,right=S.length,center,c;
		for(;left<right;c<b?left=center+1:right=center)
			if((c=S[center=left+right>>1].ch)===b)
				return S[center].searchPath(buf,p-1,historyLimit)
	}return this//p<0 or not found
}
set_ncSkip2(){
	let{children,depth,parent,nc,ch}=this,ncSkip=this.ncSkip=[],i=nc.length,cs=children.length;
	if(depth)
		a:for(let pn=parent.nc,p=pn.length,ps=parent.ncSkip,t,c;i;ncSkip[i]=t){
			for(t=this,c=nc[--i];p;)if(pn[--p]===c){
				t=ps[p];
				if(t.depth===depth)//find the children whose has ch
					for(let S=t.children,l=0,r=S.length,m;l<r;c<ch?l=m+1:r=m)
						if((c=S[m=l+r>>1].ch)===ch){
							t=S[m];continue a
						}
				break
			}
	}else for(;i;){
		ncSkip[--i]=this;//既定値=自身
		//対応する子nodeを探す
		for(let c=nc[i],l=0,r=cs,m;l<r;children[m].ch<c?l=m+1:r=m)
			if(children[m=l+r>>1].ch===c){
				ncSkip[i]=children[m];break
			}
	}
	for(;cs;)children[--cs].set_ncSkip2()
}
debugSize(){
	let{children}=this,l=children.length,i=0,s=1;
	for(;i<l;)s+=children[i++].debugSize();
	return s
}
allocateSize(){
	let{nc,children,cumFreq,ncSkip}=this,s=nc.length+(cumFreq.length+ncSkip.length+7<<2),i=children.length;
	for(;i;)s+=children[--i].allocateSize();
	return s
}
freeChildren(){
	for(var{children}=this,i=children.length;i;)children[--i].freeChildren();
	children.length=0
}
freeAll(){
	for(let{children}=this,i=children.length;i;delete children[i])
		children[--i].freeAll()
}}
//spEModel
let m_number,totalNumber;
class eModel{
constructor(fileSize,bufSize,op={interval:256}){
	if((op.interval>>>=0)<256)op.interval=256;
	if(op.interval>fileSize)op.interval=fileSize;
	if(op.interval>1<<20)op.interval=1<<20;
	this.fileSize=fileSize;this.interval=op.interval;
	this.pos=new Uint32Array(this.bufSize=bufSize);
	this.freq=new Uint32Array(256)
}
makeModel(buf){
	var{freq,pos,bufSize}=this,i=0,c=buf[0],freq2=new Uint32Array(65536);
	for(freq.fill(0),freq2[c<<8]++;++i<bufSize;freq[c]++)freq2[c|(c=buf[i])<<8]++;
	let max=c=freq2[i=0];
	for(this.buf=buf;c<bufSize;c=freq2[i]+=c)
		if(freq2[++i]>max)max=freq2[i];
	for(i=bufSize;i>1;)
		pos[--freq2[buf[--i]<<8|buf[i-1]]]=i;
	freq[buf[pos[--freq2[buf[0]<<8]]=0]]++;
	let root=new node(freq,0,null,0);
	this.makeTree(root,0,bufSize,0,new Uint32Array(max));
	return root
}
makeTree(parent,start,last,depth,work){
	let{buf,freq,pos,bufSize}=this,i=start;
	pos[i]<depth&&start++;
	for(let cur=buf[pos[i]+1];++i<last&&cur===buf[pos[i]+1];);
	if(i==last)return;//everything is same.we don't have to check more.
	if(depth>1){//We already sort depth 0,1
		let r=last-start,j=0;i=start;
		if(r<32)//Insertion sort
			for(;++i<last;pos[j]=r)
				for(r=pos[j=i];j>start&&buf[pos[j-1]-depth]>buf[r-depth];)
					pos[j]=pos[--j];
		else{//Bucket sort
			for(freq.fill(0);i<last;)freq[buf[pos[i++]-depth]]++;
			for(let c=freq[0];c<r;)c=freq[++j]+=c;
			for(;i>start;)work[--freq[buf[pos[--i]-depth]]]=pos[i];
			for(j=0;i<last;)pos[i++]=work[j++]
		}
	}//Check whether likelihood is increased
	let children=[],range=[],bitCount=0,gain=0;
	for(i=start;i<last;){
		let j=i,p=pos[i];
		if(p<depth)throw-1;
		if(p-depth===bufSize){i++;continue}
		let cur=buf[p-depth];
		//check how many character is same
		for(freq.fill(0);++p<bufSize&&++freq[buf[p]],++i<last&&cur===buf[(p=pos[i])-depth];);
		//j,j+1,...,i-1 is concerned
		let st=new node(freq,depth+1,parent,cur);
		if(!st.totalFreq)continue;
		children.push(st);range.push([j,i]);
		gain+=st.kl_div=st.kl_divergence(parent);
		bitCount+=st.checkBitN(parent)
	}
	bitCount+=parent.checkBitN2();
	if(gain<bitCount)return;
	parent.children=children;
	for(i=children.length;i;)
		this.makeTree(children[--i],range[i][0],range[i][1],depth+1,work)
}
compress(infp,outfp){
	let{fileSize,interval,freq}=this,i=fileSize,remainSize=i,split=i/interval>>>0,n=63,o=0,p;
	for(;i-=outfp[o++]=i&n;n=255)i/=n+1,outfp[0]+=64;
	for(i=interval,n=63,p=o;i-=outfp[o++]=i&n;n=255)i/=n+1,outfp[p]+=64;
	let inPos=0,treePos=[],e=new rangeEncoder(outfp,o),out_pos=new Uint32Array(split+1);//Position at Compressed Data;
	for(out_pos[0]=o;i<256;)freq[i++]=1;
	let base=new node(freq,-1,null,0);
	for(p=0;;){
		let shouldReadSize=remainSize>MAXBUF?MAXBUF:remainSize,A=infp.subarray(p,p+=shouldReadSize),readSize=A.length;
		if(!readSize)break;
		remainSize-=this.bufSize=readSize;
		let t=this.makeModel(A),root=t,step=0;
		t.set_ncSkip2();
		e.setup();
		for(i=0;i<readSize;){
			let c=A[i++];
			t.used=true;
			//Binary Search
			let left=0,right=t.nc.length,p;
			for(;left<right&&t.nc[p=left+right>>1]!==c;)t.nc[p]<c?left=p+1:right=p;
			if(left===right)throw"cannot found nc";
			e.encodeshift(t.cumFreq[p],t.freq[p],R_SHIFT);
			if(++step===interval){
				step=0;e.flush();e.setup();
				out_pos[++inPos]=outfp.length;
				t=root;continue
			}
			t=t.ncSkip[p]
		}
		e.flush();
		treePos.push(outfp.length);
		let rc=new riceEncode(outfp);
		root.outputRice(rc);
		rc.flush();
		rc=new rangeEncoder(outfp);
		root.outputRice4(base,rc);
		rc.flush();
		m_number=0;
		out_pos[inPos]=outfp.length
	}
	//contents of footer
	let nowPos=outfp.length,rc=new riceEncode(outfp);
	rc.unsignedcode(out_pos[0],RICE_MASK);
	let x=1/0,y=(out_pos[0]-out_pos[i=inPos])/~i>>>0,min=x,avg=y;
	for(;i;){
		let n=out_pos[i]-=out_pos[--i];
		if(n<min)min=n
	}
	//find best avg
	for(i=min;i<y;i++){
		let a=0,c=0;
		for(;a<inPos;){
			let n=out_pos[++a]-i,b=0;
			if(n<0)n=~n;
			for(;n>>++b;);
			c+=(b>>2)+(b>1?b+3:5)
		}
		if(c<x)x=c,avg=i
	}
	rc.code(avg,RICE_MASK);
	for(i=0;i<inPos;)rc.code(out_pos[++i]-avg,2);
	rc.flush(i=0);out_pos=[];
	for(x of treePos)
		for(n=63,p=i;x-=out_pos[i++]=x&n;n=255)x/=n+1,out_pos[p]+=64;
	for(n=63,p=i;nowPos-=out_pos[i++]=nowPos&n;n=255)nowPos/=n+1,out_pos[p]+=64;
	for(o=outfp.length;i;)outfp[o++]=out_pos[--i];
	return outfp
}}
let CACHE;
function readTree(infp,filePos,base){
	infp.p=filePos;
	let rc=new riceDecode(infp),t=new dNode;
	totalNumber=0;
	t.inputRice(rc,0);
	let rd=new rangeDecoder;
	rd.setup(infp,infp.p=rc.p);
	t.inputRice4(base,rd);
	t.set_ncSkip2();
	if(CACHE)t.freeChildren();
	return t
}
function SPPMd(infp,opt={}){
	let a=0,b=infp[a++],c=64,fileSize=b&63,z=infp.length,start=opt.start>>>0,last=opt.last>>>0,o=0,outfp=[],outPos,treePos=[];
	for(b>>=6;b--;c*=256)fileSize+=infp[a++]*c;
	b=infp[a++],c=64;
	let interval=b&63,rootGsize=(fileSize+MAXBUF-1)/MAXBUF>>>0;
	for(b>>=6;b--;c*=256)interval+=infp[a++]*c;
	if(start>fileSize)return[];
	if(start>=last)last=fileSize;
	if(interval>fileSize)interval=fileSize;
	if(interval>MAXBUF)interval=MAXBUF;
	{
		let i=rootGsize,d;a=z;
		for(;i--;treePos.push(d)){
			b=infp[--a],c=64,d=b&63;
			for(b>>=6;b--;c*=256)d+=c*infp[--a]
		}
		b=infp[--a],c=64,d=b&63;
		for(b>>=6;b--;c*=256)d+=c*infp[--a];
		infp.p=d;
		let rc=new riceDecode(infp);
		outPos=new Uint32Array(c=fileSize/interval+1>>>0);
		outPos[i=0]=rc.unsigneddecode(RICE_MASK);
		for(b=rc.decode(RICE_MASK);++i<c;)outPos[i]=outPos[i-1]+rc.decode(2)+b
	}
	let base=new dNode,d=new rangeDecoder;
	base.nc=Uint8Array.from(Array(base.size=256),(a,b)=>b);
	let beforeRoot=-1,root,nowBlock=start/interval>>>0,startOffset=start%interval,
		step=0,started=0,remain=last-start,currentRoot=start/MAXBUF>>>0;

	if(currentRoot^beforeRoot)root=readTree(infp,treePos[beforeRoot=currentRoot],base);
	infp.p=outPos[nowBlock];
	d.setup(infp);
	for(let t=root;;){
		let c=d.getCharacterShift2(t.cumFreq,t.size,R_SHIFT),cp=t.nc[c];
		step++;
		if(!started){
			if(step!=startOffset+1){t=t.ncSkip[c];continue}
			started=1
		}
		outfp[o++]=cp;
		if(!--remain)break;
		if(step===interval){
			nowBlock++;step=0;currentRoot=(last-remain)/MAXBUF>>>0;
			infp.p=d.p;
			if(currentRoot^beforeRoot){
				if(!CACHE)root.freeAll();
				root=readTree(infp,treePos[beforeRoot=currentRoot],base);
				infp.p=outPos[nowBlock]
			}
			d.setup(infp);t=root;continue
		}
		t=t.ncSkip[c]
	}return outfp
}
function SPPMe(A,interval){
	let z=A.length,e=new eModel(z,z,{interval}),O=[];
	e.compress(A.buffer?A:new Uint8Array(A),O);
	return O
}