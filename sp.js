/*sp 2025 10 25
partial decodable compression by static PPM.
it based on sp v0.2(C)2004 11 28 Daisuke Okanohara*/
{const RICE_MASK=3,R_SHIFT=14,MAXBUF=1<<20,MAXDEPTH=10;
function rightbits(len,c){return(1<<len)-1&c}
class riceEncode{
constructor(fp){
	this.O=fp;this.c=32;this.b=0;this.p=fp.length
}
putbits(n,l){
	for(;l>=this.c;this.c=32)
		l-=this.c,
		this.b|=rightbits(this.c,n>>>l),
		this.O[this.p++]=this.b&255,
		this.O[this.p++]=this.b>>8&255,
		this.O[this.p++]=this.b>>16&255,
		this.O[this.p++]=this.b>>>24,
		this.b=0;
	this.b|=rightbits(l,n)<<(this.c-=l);
}
putbit(b){
	this.c--;
	if(b)this.b|=1<<this.c;
	if(!this.c)
		this.O[this.p++]=this.b&255,
		this.O[this.p++]=this.b>>8&255,
		this.O[this.p++]=this.b>>16&255,
		this.O[this.p++]=this.b>>>24,
		this.b=0,this.c=32
}
code(n,m){
	this.putbit(n<0);
	if(n<0)n=~n;
	for(var b=0;n>>++b;);
	this.ucode(b-1,m);
	b>1?this.putbits(n,b-1):this.putbit(n>0);
}
ucode(n,m){
	this.putbits(0,n>>>m);this.putbit(1);this.putbits(n,m)
}
flush(){this.putbits(0,31)}
}
class riceDecode{
constructor(fp){this.I=fp;this.p=fp.p>>>0;this.c=this.b=0}
getbit(){
	if(this.c--)return this.b>>>this.c&1;
	this.c=31;
	this.b=this.I[this.p++]|this.I[this.p++]<<8|this.I[this.p++]<<16|this.I[this.p++]<<24;
	return this.b>>>31&1
}
getbits(l,n){
	for(;l>this.c;this.c=32)
		l-=this.c,
		n|=rightbits(this.c,this.b)<<l,
		this.b=this.I[this.p++]|this.I[this.p++]<<8|this.I[this.p++]<<16|this.I[this.p++]<<24;
	return n|rightbits(l,this.b>>>(this.c-=l))
}
decode(b){
	let m=this.getbit(),n=this.udecode(b);
	n=n?1<<n|this.getbits(n):this.getbit();
	return m?~n:n
}
udecode(m){
	for(var n=0;!this.getbit();)n++;
	return n<<m|this.getbits(m)
}}
//range coder
class rangeEncoder{
constructor(outfp,o){this.o=o||outfp.length;this.setup(this.outfp=outfp)}
pc(c){this.outfp[this.o++]=c}
setup(){this.low=this.count=0;this.range=-1>>>0}
flush(i){for(i=4;i--;this.low<<=8)this.pc(this.low>>>24)}
// normal coding
encode(c,f,t){
	let r=this.range/t>>>0,l=this.low+r*c;
	if(!r)throw"range error";
	for(r*=f;!((l^l+r)>>24)||r<65536&&(r=-l&65535);r*=256)this.pc(l>>>24),l<<=8;
	this.range=r;this.low=l
}
// using shift instead of dividing
encodeshift(c,f,s){
	let r=this.range>>>s,l=this.low+r*c;
	if(!r)throw"range error";
	for(r*=f;!((l^l+r)>>24)||r<65536&&(r=-l&65535);r*=256)this.pc(l>>>24),l<<=8;
	this.range=r;this.low=l
}
// coding number lower than maxnumber
encodeNumber(n,max){this.encode(n,1,max)}
}
class rangeDecoder{
setup(In){this.infp=In;this.p=In.p>>>0;this.init()}
init(){
	this.range=-1>>>0;this.low=0;
	for(let i=4;i--;)this.code=this.code<<8|this.infp[this.p++];
}
// normalize range and low
normalize(){
	for(;!((this.low^this.low+this.range)>>24)||this.range<65536&&(this.range=-this.low&65535);this.range*=256)
		this.code=this.code<<8|this.infp[this.p++],this.low<<=8;
}
// get next accumurate prob using shift
getfreqshift(s){
	let t=(this.code-this.low>>>0)/(this.range>>>=s)>>>0;
	if(t>=1<<s)throw"totfreq error";
	return t
}
// get next accumurate prob
getfreq(sum){
	let f=(this.code-this.low>>>0)/(this.range=this.range/sum>>>0)>>>0;
	if(f>=sum)throw"totfreq error";
	return f
}
basic_getCharacter(C,n){
	let f=(this.code-this.low>>>0)/this.range>>>0,i=0;
	if(f>=C[n])throw"basic_getCharacter error";
	for(;C[++i]<=f;);
	f=C[i-1];this.low+=f*this.range;
	this.range*=C[i]-f;
	this.normalize();
	return--i
}
basic_getCharacter2(C,n){
	let f=(this.code-this.low>>>0)/this.range>>>0,i=0,j=n,k;
	if(f>=C[n])throw"basic_getCharacter error";
	for(;i<j;C[k+1]>f?j=k:i=k+1)k=i+j>>1;
	f=C[i];this.low+=f*this.range;
	this.range*=C[i+1]-f;
	this.normalize();
	return i
}
getCharacterShift2(C,n,s){
	this.range>>>=s;
	return this.basic_getCharacter2(C,n);
}
getCharacter(C,n){
	this.range=this.range/C[n]>>>0;
	return this.basic_getCharacter(C,n)
}
getCharacterShift(C,n,s){
	this.range>>>=s;
	return this.basic_getCharacter(C,n);
}
decodeNumber(max){
	let n=(this.code-this.low>>>0)/(this.range=this.range/max>>>0)>>>0;
	this.low+=n*this.range;
	this.normalize();
	return n
}
decode(c,f){
	this.low+=c*this.range;
	this.range*=f;
	this.normalize()
}}
//range coder util
function normalize(F,s){
	let max=1<<s,l=F.length,i=l;
	for(s=0;i;)s+=F[--i];
	if(!s)return 0;
	let c=0,t;
	for(i=l;i;)
		if(t=F[--i])
			t*=max,c+=F[i]=t<s?1:t/s>>>0;
	if(c>max){
		//Decreasing each frequency
		for(t=c-max;t;)
			for(i=0;i<l;i++)
				if(F[i]>1){--F[i];if(!--t)break}
	}else if(c<max)
		//Increasing each frequency
		for(t=max-c;t;)
			for(i=0;i<l;i++)
				if(F[i]){++F[i];if(!--t)break}
	return s
}
function normalize2(F,s,size){
	let max=1<<s,i=size,c=0,t;
	for(s=0;i;)s+=F[--i];
	if(!s)return 0;
	for(;i<size;)t=F[i]*max,c+=F[i++]=t<s?1:t/s>>>0;
	if(c>max){
		for(t=c-max;t;)
			for(i=0;i<size;i++)
				if(F[i]>1){--F[i];if(!--t)break}
	}else if(c<max)
		for(t=max-c;t;)
			for(i=0;i<size;i++)
				if(F[i]){++F[i];if(!--t)break}
	return s
}
function makeCumFreq_dec(C,s){
	let max=1<<s,l=C.length,i=l;
	for(s=C[l-1]=0;i;)s+=C[--i];
	if(!s)return;
	let c=0,t;
	for(i=l--;i;)if(t=C[--i])
		t*=max,c+=C[i]=t<s?1:t/s>>>0;
	if(c>max){
		for(t=c-max;t;)
			for(i=0;i<l;i++)
				if(C[i]>1){--C[i];if(!--t)break}
	}else if(c<max)
		for(t=max-c;t;)
			for(i=0;i<l;i++)
				if(C[i]){++C[i];if(!--t)break}
	for(i=t=0,l++;i<l;t+=c)c=C[i],C[i++]=t
}
function makeCumFreq_dec1(F,C){
	for(let i=0,c=C[0],size=F.length;i<size;)C[i+1]=c+=F[i++]
}
function makeCumFreq(F,C){
	for(let i=0,c=C[0],size=F.length;++i<size;)C[i]=c+=F[i-1]
}
//spNode
class node{
constructor(F,d,dad,c){
	this.sum=this.Sum=this.toBit=this.kl_div=this.used=this.mearged=this.initialized=0,
	this.meargedNumber=-1,this.depth=d,this.dad=dad,this.ch=c;
	let i=256,min=-1>>>1,n=c=0,f;
	for(this.son=[];i--;)if(f=F[i]){
		n++;c+=f;
		if(f<min)min=f
	}
	this.nc=new Uint8Array(n);this.Sum=c;
	this.hit=new(c>>16?Uint32Array:Uint16Array)(n);
	for(c=0;c<n;)if(f=F[++i]){
		this.nc[c]=i;
		if(f>2){
			let k=0;
			for(;f>>++k;);
			f>>=k-=k>>1;f<<=k
		}
		this.sum+=this.hit[c++]=f
	}
	this.size=n
}
kl_divergence(dad){
	let{hit,nc,sum}=this,kl=0,pf=dad.hit,pn=dad.nc,pt=dad.sum;
	for(let i=0,j=0,l=hit.length,m=pf.length,c;i<l;kl+=c*Math.log(c/(pf[j]/pt))){
		//search parent
		for(c=nc[i];pn[j]!==c&&j<m;)j++;
		if(j===m)throw"kl_dirvergence error";
		c=hit[i++]/sum
	}
	return(kl/Math.LN2)*this.Sum
}
riceUnsignedN(n,m){return(n>>m)+m+1}
checkBitN(dad){
	let{hit,nc}=this,i=0,n0=nc.length,b=this.riceUnsignedN(n0,RICE_MASK),c=0,pn=dad.nc,total=pn.length,ltotal=total&&Math.log(total),ln0=n0&&Math.log(n0),ln1=total>n0&&Math.log(total-n0);
	b+=(total*ltotal-n0*ln0-(total-n0)*ln1)/Math.LN2>>>0;
	if(n0>1)for(;i<total&&c<n0;)
		if(pn[i++]===nc[c]){
			let t=hit[c++]-1,k=0;
			for(;t;k++)t>>=1;
			b+=this.riceUnsignedN(k,2);
			if(k)b+=k>>1
		}
	return++b
}
checkBitN2(){
	let n0=this.son.length,ln0=n0&&Math.log(n0),ln1=256>n0&&Math.log(256-n0),b=this.riceUnsignedN(n0,RICE_MASK);
	return b+=(1419.565425786768-n0*ln0-(256-n0)*ln1)/Math.LN2>>>0
}
checkBitN3(){
	let b=0,p=0,t=this.sum,i=this.nc.length,F=this.hit;
	for(;t;p++)t>>=1;p>>=1;
	if(i^1)b+=this.riceUnsignedN(p,RICE_MASK);
	for(;i;)b+=this.riceUnsignedN(F[--i]-1,p);
	return b
}
getSize(){
	let i,n=1;
	for(i of this.son)n+=i.getSize();
	return n
}
free(){
	for(let i of this.son)i.free();delete this.son
}
getNonMeargeSize(){
	let i,n=this.mearged&1;
	for(i of this.son)n+=i.getNonMeargeSize();
	return n
}
getUsedSize(){
	let i,n=this.used&1;
	for(i of this.son)n+=i.getUsedSize();
	return n
}
searchPath(B,p,historyLimit){
	if(p>=historyLimit)//binary search
		for(let b=B[p],S=this.son,l=0,r=S.length,m;l<r;S[m].ch<b?l=m+1:r=m)
			if(S[m=l+r>>1].ch===b)return S[m].searchPath(B,p-1,historyLimit);
	//p<0 or not found
	return this
}
outputRice(rc,cs,m){
	let{nc,son,Hit,depth}=this,n=nc.length,l=this.used;
	rc.ucode(n-1,cs[depth]);
	rc.putbit(l);
	if(l&&n>1){
		let c0=0,c1=0,i=n;
		//cost of log2
		for(;i;){
			let t=Hit[--i],k=0;
			for(;t>>++k;);
			c0+=(--k>>1)+2,c1+=(k>>2)+3
		}
		rc.putbit(c0=0|c0>c1);c0++;
		for(;i<n;){
			let t=Hit[i++],k=0;
			for(;t>>++k;);
			rc.ucode(k-1,c0);
			if(k>2)l=k>>1,rc.putbits(t>>l+(k&1),l)
		}
	}
	if(m)return this.meargedNumber=m_number++;
	n=son.length;rc.putbit(n>0);
	if(n)for(rc.ucode(n-1,RICE_MASK);n;)son[--n].outputRice(rc,cs)
}
outputRice4(dad,ra,m){
	let{nc,son}=this,pn=dad.nc,n=nc.length,sum=pn.length,i=0,c=0;
	for(;c<n&&n-c-sum+i;)
		if(pn[i++]===nc[c])ra.encode(0,n,sum),c++;
		else ra.encode(n,sum-n,sum);
	if(m)return this.meargedNumber=m_number++;
	m=ra.cs||(ra.cs=1+nc.at(-1));
	if(n=son.length){
		for(i=c=0;c<n&&n-c-m+i;)
			if(son[c].ch===i++)
				ra.encode(0,n,m),
				son[c++].outputRice4(this,ra);
			else ra.encode(n,m-n,m);
		for(;c<n;)son[c++].outputRice4(this,ra)
	}
}
mearge(){
	let c=[],cs=0,i;
	for(i of this.son)i.son.length?i.mearge():c[cs++]=i;
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
				let F=a.hit,N=a.nc,k=F.length,l=k;
				for(;k;)cf[N[--k]]+=F[k];
				F=b.hit,N=b.nc,l+=k=F.length;
				for(;k;)cf[N[--k]]+=F[k];
				let t=new node(cf,a.depth,a.dad,a.ch),gain=a.kl_divergence(t)+b.kl_divergence(t);
				t.toBit=t.checkBitN3();
				let bitCount=a.toBit+b.toBit-t.toBit+l-t.hit.length+1;
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
setNcSkip2(){
	if(this.initialized)return;
	let{hit,son,ch,nc,depth,dad}=this,i=nc.length,cs=son.length,ncSkip=this.ncSkip=[];
	this.initialized=1;
	this.Hit=hit.slice();
	normalize(hit,R_SHIFT);
	makeCumFreq(hit,this.cumFreq=new Uint32Array(i));
	if(depth)// 子node
		a:for(let pn=dad.nc,ps=dad.ncSkip,p=pn.length,t,c;i;ncSkip[i]=t){
			for(c=nc[--i];pn[--p]!==c;);
			t=ps[p];
			if(t.depth===depth)//find the son whose has ch
				for(let S=t.son,l=0,r=S.length,m;l<r;c<ch?l=m+1:r=m)
					if((c=S[m=l+r>>1].ch)===ch){
						t=S[m];continue a
					}
	}else for(;i;){// 根
		ncSkip[--i]=this;//既定値=自身
		//対応する子nodeを探す
		for(let c=nc[i],l=0,r=cs,m;l<r;son[m].ch<c?l=m+1:r=m)
			if(son[m=l+r>>1].ch===c){
				ncSkip[i]=son[m];break
			}
	}
	for(;cs;)son[--cs].setNcSkip2()
}}
//spDNode
class dNode{
constructor(){}
inputRice(rc,d,cs){
	let size=rc.udecode(cs[d])+1,S=this.son=[];
	this.depth=d;
	this.No=totalNumber++;
	this.nc=new Uint8Array(this.size=size);
	if(rc.getbit()){
		let cf=this.cumFreq=new Uint32Array(size+1);
		if(size<2)cf[0]=1;
		else for(let i=0,k,b,l=rc.getbit()+1;i<size;cf[i++]=k>2?rc.getbits(b=k>>1)<<b+(k&1):k)
			k=rc.udecode(l)+1;
		makeCumFreq_dec(cf,R_SHIFT)
	}
	if(rc.getbit())
		for(let i=rc.udecode(RICE_MASK)+1;i;)
			(S[--i]=new dNode).inputRice(rc,d+1,cs)
}
inputRice4(dad,rd){
	this.dad=dad;
	let{nc,son}=this,pn=dad.nc,m=pn.length,n=nc.length,C=new Uint16Array([0,n,m]),i=0,c=0;
	for(;c<n&&n-c-m+i;i++)
		if(!rd.getCharacter(C,2))nc[c++]=pn[i];
	for(;c<n;)nc[c++]=pn[i++];
	m=rd.cs||(rd.cs=1+nc[c-1]);
	if(n=son.length){
		C[2]=m;C[1]=n;
		for(C[0]=i=c=0;c<n&&n-c-m+i;i++)
			if(!rd.getCharacter(C,2))
				son[c].ch=i,
				son[c++].inputRice4(this,rd);
		for(;c<n;son[c++].inputRice4(this,rd))son[c].ch=i++
	}
}
searchPath(B,p,historyLimit){
	if(p>=Math.max(historyLimit,0)){
		//binary search
		for(let S=this.son,b=B[p],l=0,r=S.length,m,c;l<r;c<b?l=m+1:r=m)
			if((c=S[m=l+r>>1].ch)===b)
				return S[m].searchPath(B,p-1,historyLimit)
	}return this//p<0 or not found
}
setNcSkip2(){
	let{son,depth,dad,nc,ch}=this,ncSkip=this.ncSkip=[],i=nc.length,cs=son.length;
	if(depth)// 子node
		a:for(let pn=dad.nc,p=pn.length,ps=dad.ncSkip,t,c;i;ncSkip[i]=t){
			for(c=nc[--i];pn[--p]!==c;);
			t=ps[p];
			if(t.depth===depth)//find the son whose has ch
				for(let S=t.son,l=0,r=S.length,m;l<r;c<ch?l=m+1:r=m)
					if((c=S[m=l+r>>1].ch)===ch){
						t=S[m];continue a
					}
	}else for(;i;){// 根
		ncSkip[--i]=this;//既定値=自身
		//対応する子nodeを探す
		for(let c=nc[i],l=0,r=cs,m;l<r;son[m].ch<c?l=m+1:r=m)
			if(son[m=l+r>>1].ch===c){
				ncSkip[i]=son[m];break
			}
	}
	for(;cs;)son[--cs].setNcSkip2()
}
allocateSize(){
	let{nc,son,cumFreq,ncSkip}=this,s=nc.length+(cumFreq.length+ncSkip.length+7<<2),i=son.length;
	for(;i;)s+=son[--i].allocateSize();
	return s
}
freeSon(){
	for(var{son}=this,i=son.length;i;)son[--i].freeSon();
	son.length=0
}
freeAll(){
	for(let{son}=this,i=son.length;i;delete son[i])
		son[--i].freeAll()
}}
//spEModel
let m_number,totalNumber;
class eModel{
constructor(inSize,bs,op={interval:256}){
	if((op.interval>>>=0)<256)op.interval=256;
	if(op.interval>inSize)op.interval=inSize;
	if(op.interval>1<<20)op.interval=1<<20;
	this.inSize=inSize;this.interval=op.interval;
	this.P=new Uint32Array(this.bs=bs);
	this.F=new Uint32Array(256);this.cost=[new Uint32Array(4)]
}
makeModel(B){
	let{F,P,bs,cost}=this,i=0,c=B[0],C=new Uint32Array(65536);
	cost.length=1;cost[0].fill(0);
	for(F.fill(0),C[c<<8]++;F[c]++,++i<bs;)C[c|(c=B[i])<<8]++;
	let max=c=C[i=0];
	for(this.B=B;c<bs;c=C[i]+=c)
		if(C[++i]>max)max=C[i];
	for(i=bs;i>1;)P[--C[B[--i]<<8|B[i-1]]]=i;
	let root=new node(F,0,null,P[--C[B[0]<<8]]=0);
	this.makeTree(root,0,bs,0,new Uint32Array(max));
	return root
}
makeTree(dad,a,z,d,W){
	let{B,F,P,bs,cost}=this,i=a;
	P[i]<d&&a++;
	for(let c=B[P[i]+1];++i<z&&c===B[P[i]+1];);
	if(i==z)return;//everything is same. no need to check more
	if(d>1){//We already sort depth 0,1
		let r=z-a,j=0;i=a;
		if(r<32)//Insertion sort
/*			for(;++i<z;P[j]=r)
				for(r=P[j=i];j>a&&B[P[j-1]-d]>B[r-d];)
					P[j]=P[--j];*/
			for(let a=i,c,k;++i<z;P[j]=r)
				if(r=P[j=k=i],(c=B[r-d])<B[P[a]-d])for(;j>a;)P[j]=P[--j];
				else for(;c<B[P[--k]-d]&&j>a;)P[j]=P[--j];
		else{//Bucket sort
			for(F.fill(0);i<z;)F[B[P[i++]-d]]++;
			for(let c=F[0];c<r;)c=F[++j]+=c;
			for(;i>a;W[--F[B[r-d]]]=r)r=P[--i];
			for(j=0;i<z;)P[i++]=W[j++]
		}
	}//Check whether likelihood is increased
	let son=[],range=[],bc=0,gain=0;
	for(;a<z;){
		let p=P[i=a];
		if(p<d)throw-1;
		if(p-d===bs){a++;continue}
		let c=B[p-d];
		//check how many character is same
		for(F.fill(0);++p<bs&&++F[B[p]],++a<z&&c===B[(p=P[a])-d];);
		//i,i+1,...,a-1 is concerned
		let st=new node(F,d+1,dad,c);
		if(st.sum){
			son.push(st);range.push(i,a);
			gain+=st.kl_div=st.kl_divergence(dad);
			bc+=st.checkBitN(dad)
		}
	}
	if(gain<bc+dad.checkBitN2())return;
	let C=cost[d]??=new Uint32Array(4);
	dad.son=son;
	for(i=son.length;i;){
		let S=son[--i];
		for(a=4,bc=S.nc.length-1;a;)C[--a]+=(bc>>a+1)+a+2;
		this.makeTree(S,range[i*2],range[i*2+1],d+1,W)
	}
}
async compress(In,O,rate){
	let{inSize,interval,F}=this,i=inSize,rest=i,n=63,o=0,p;
	for(;i-=O[o++]=i&n;n=255)i/=n+1,O[0]+=64;
	for(i=interval,n=63,p=o;i-=O[o++]=i&n;n=255)i/=n+1,O[p]+=64;
	let op=0,Tp=[],e=new rangeEncoder(O,o),Op=new Uint32Array(inSize/interval+1>>>0);//Position at Compressed Data;
	for(Op[0]=o;i<256;)F[i++]=1;
	let base=new node(F,-1,null,0),fn=b=>wait0(c=>b(rate(i,inSize))),st=+new Date;
	for(p=0;;){
		let readSize=rest>MAXBUF?MAXBUF:rest,A=In.subarray(p,p+=readSize);
		if(!readSize)break;
		rest-=this.bs=readSize;
		let t=this.makeModel(A),root=t,step=0,I=new Int8Array([0,1,2,3]);
		t.setNcSkip2();
		e.setup();
		for(i=0;i<readSize;i&32767||Date.now()-st<100||await new Promise(fn,st=+new Date)){
			t.used=1;
			//Binary Search
			let c=A[i++],l=0,r=t.size,p=0;
			if(r>1){
				for(;l<r&&t.nc[p=l+r>>1]!==c;)t.nc[p]<c?l=p+1:r=p;
				if(l===r)throw"cannot found nc";
				e.encodeshift(t.cumFreq[p],t.hit[p],R_SHIFT)
			}
			if(++step===interval){
				step=0;e.flush();e.setup();
				Op[++op]=O.length;
				t=root;continue
			}
			t=t.ncSkip[p]
		}
		e.flush();
		Tp.push(O.length);
		let rc=new riceEncode(O),C=this.cost,cs=[5,1,1,1,1];
		if(readSize>>9){
			rc.ucode(C.length-1,2);cs[0]=6;
			//write optimized arguments of outputRice
			for(let b=i=0,c;c=C[i++];cs[i]=b+1)rc.putbits(b=I.sort((a,b)=>c[a]-c[b])[0],2)
		}
		root.outputRice(rc,cs);rc.flush();
		rc=new rangeEncoder(O);
		root.outputRice4(base,rc);rc.flush();
		m_number=0;
		Op[op]=O.length
	}
	//contents of footer
	let np=O.length,rc=new riceEncode(O);
	let x=1/0,v=(Op[0]-Op[i=op])/~i>>>0,min=x,avg=v;
	for(rc.ucode(Op[0],RICE_MASK);i;){
		let n=Op[i]-=Op[--i];
		if(n<min)min=n
	}
	//find best avg
	for(;min<v--;){
		let c=i=0;
		for(;i<op;){
			let n=Op[++i]-v,b=0;
			if(n<0)n=~n;
			for(;n>>++b;);
			c+=(b>>2)+(b>1?b+3:5)
		}
		if(c<x)x=c,avg=v
	}
	if(inSize>interval)
		for(i=0,rc.code(avg,RICE_MASK);i<op;)rc.code(Op[++i]-avg,2);
	rc.flush(i=0);Op=[];
	for(x of Tp)
		for(n=63,p=i;x-=Op[i++]=x&n;n=255)x/=n+1,Op[p]+=64;
	for(n=63,p=i;np-=Op[i++]=np&n;n=255)np/=n+1,Op[p]+=64;
	for(o=O.length;i;)O[o++]=Op[--i];
	return O
}}
let CACHE;
function readTree(infp,p,base,rest){
	infp.p=p;
	let rc=new riceDecode(infp),t=new dNode,C=[6,1,1,1,1];
	if(rest>>9)for(let a=0,b=rc.udecode(2)+1;a<b;)C[++a]=rc.getbits(2)+1;
	else C[0]=5;
	t.inputRice(rc,totalNumber=0,C);
	let rd=new rangeDecoder;
	rd.setup(infp,infp.p=rc.p);
	t.inputRice4(base,rd);
	t.setNcSkip2();
	if(CACHE)t.freeSon();
	return t
}
this.SPPMd=async function(In,opt={},done,rate){
	let a=0,b=In[a++],c=64,inSize=b&63,z=In.length,start=opt.start>>>0,last=opt.last>>>0,o=0,O=[],Op,Tp=[];
	for(b>>=6;b--;c*=256)inSize+=In[a++]*c;
	b=In[a++],c=64;
	let interval=b&63,rootGsize=(inSize+MAXBUF-1)/MAXBUF>>>0;
	for(b>>=6;b--;c*=256)interval+=In[a++]*c;
	if(start>inSize)return O;
	if(start>=last)last=inSize;
	if(interval>inSize)interval=inSize;
	if(interval>MAXBUF)interval=MAXBUF;
	{//read footer
		let i=rootGsize,d;
		for(a=z;i--;Tp.push(d)){
			b=In[--a],c=64,d=b&63;
			for(b>>=6;b--;c*=256)d+=c*In[--a]
		}
		b=In[--a],c=64,d=b&63;
		for(b>>=6;b--;c*=256)d+=c*In[--a];
		In.p=d;
		let rc=new riceDecode(In);
		Op=new Uint32Array(c=inSize/interval+1>>>0);
		Op[i=0]=a=rc.udecode(RICE_MASK);
		if(interval<inSize)
			for(b=rc.decode(RICE_MASK);++i<c;)Op[i]=a+=rc.decode(2)+b;
		else Op[1]=d+a
	}
	let base=new dNode,d=new rangeDecoder,fn=b=>wait0(c=>b(rate(o,inSize))),st=+new Date;
	base.nc=Uint8Array.from(Array(base.size=256),(a,b)=>b);
	let beforeRoot=-1,root,Bn=start/interval>>>0,startOffset=start%interval,
		step=0,started=0,rest=last-start,currentRoot=start/MAXBUF>>>0;

	if(currentRoot^beforeRoot)root=readTree(In,Tp[beforeRoot=currentRoot],base,rest+start);
	In.p=Op[Bn];d.setup(In);
	for(let t=root;;o&32767||Date.now()-st<100||await new Promise(fn,st=+new Date)){
		let c=t.size,cp=t.nc[c=c>1?d.getCharacterShift2(t.cumFreq,c,R_SHIFT):0];
		step++;
		if(!started){
			if(step!=startOffset+1){t=t.ncSkip[c];continue}
			started=1
		}
		O[o++]=cp;
		if(!--rest)break;
		if(step===interval){
			Bn++;step=0;currentRoot=(last-rest)/MAXBUF>>>0;
			In.p=d.p;
			if(currentRoot^beforeRoot){
				if(!CACHE)root.freeAll();
				root=readTree(In,Tp[beforeRoot=currentRoot],base,rest+start);
				In.p=Op[Bn]
			}
			d.setup(In);t=root;continue
		}
		t=t.ncSkip[c]
	}done&&done(O,z,o);
	return O
};
this.SPPMe=async function(A,interval,done,rate){
	let z=A.length,e=new eModel(z,z,{interval}),O=[];
	await e.compress(A.buffer?A:new Uint8Array(A),O,rate);
	done&&done(O,z,O.length);
	return O
}}