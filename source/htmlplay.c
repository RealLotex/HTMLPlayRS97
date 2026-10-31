/* HTMLPlay RS97 - small local HTML/CSS/JS game runtime for RetroFW.
 * Copyright (c) 2026. MIT License. Direct Linux FB RGB565 + evdev, MIPS o32.
 * Not a standards-compliant browser. No network, audio, images, full DOM or JS engine.
 */
#ifdef TEST_HOST
#include <stdio.h>
#include <stdlib.h>
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
#define W 320
#define H 240
#define FIX 256
#define MAX_HTML 131072
#define MAX_JS 65536
#define MAX_FILES 40
#define MAX_VARS 160
#define MAX_FUNCS 20
#define MAX_TEXT 240
#define MAX_INSTRUCTIONS 20000
static u16 fb[W*H];
static char html[MAX_HTML], script[MAX_JS];
static int script_len, bg=0x142; /* RGB565 */
static int canvas_w=320,canvas_h=240;
static char games[MAX_FILES][80];
static int game_count=0, selection=0;
static int down[512], keys[128];
static int running=1, in_game=0, frames=0;
static int fill_color=0xffff, stroke_color=0xffff;

void *memset(void *v,int c,unsigned long n) {u8*p=(u8*)v;while(n--)*p++=(u8)c;return v;}
void *memcpy(void *d,const void*s,unsigned long n) {u8*p=(u8*)d;const u8*q=(const u8*)s;while(n--)*p++=*q++;return d;}
static int strlen_(const char*s){int n=0;while(s&&s[n])n++;return n;}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}
static int ieq(const char*a,const char*b){while(*a&&*b){int x=*a++,y=*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}return !*a&&!*b;}
static int prefix(const char*s,const char*p){while(*p)if(*s++!=*p++)return 0;return 1;}
static int issp(int c){return c==' '||c=='\r'||c=='\n'||c=='\t';}
static int isalpha(int c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'||c=='$';}
static int isdigit(int c){return c>='0'&&c<='9';}
static int ends(const char*s,const char*ext){int a=strlen_(s),b=strlen_(ext);return a>=b&&ieq(s+a-b,ext);}
static void strcopy(char*d,const char*s,int cap){int i=0;while(s&&s[i]&&i<cap-1){d[i]=s[i];i++;}d[i]=0;}
static int clamp(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static int absolute(int x){return x<0?-x:x;}
static int parse_int(const char*s){int n=0;while(isdigit(*s)){n=n*10+(*s++-'0');}return n;}
static int hex(int c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static int color565(int r,int g,int b){return (clamp(r,0,255)>>3)<<11|(clamp(g,0,255)>>2)<<5|(clamp(b,0,255)>>3);}
static int colorparse(const char*s){if(!s||!*s)return 0xffff;if(*s=='#'){s++;int n=strlen_(s);if(n==3){int a=hex(s[0]),b=hex(s[1]),c=hex(s[2]);if(a>=0&&b>=0&&c>=0)return color565(a*17,b*17,c*17);}if(n>=6){int a=hex(s[0])*16+hex(s[1]),b=hex(s[2])*16+hex(s[3]),c=hex(s[4])*16+hex(s[5]);if(a>=0&&b>=0&&c>=0)return color565(a,b,c);}return 0xffff;}
 if(ieq(s,"white"))return color565(255,255,255);if(ieq(s,"black"))return 0;if(ieq(s,"red"))return color565(255,0,0);if(ieq(s,"blue"))return color565(0,0,255);if(ieq(s,"green"))return color565(0,180,0);if(ieq(s,"yellow"))return color565(255,255,0);if(ieq(s,"orange"))return color565(255,150,0);if(ieq(s,"purple"))return color565(180,0,200);if(ieq(s,"cyan"))return color565(0,255,255);return 0xffff;}
static void rect(int x,int y,int w,int h,int c){if(w<0){x+=w;w=-w;}if(h<0){y+=h;h=-h;}int x0=clamp(x,0,W),y0=clamp(y,0,H),x1=clamp(x+w,0,W),y1=clamp(y+h,0,H);for(int j=y0;j<y1;j++){u16*p=fb+j*W+x0;for(int i=x0;i<x1;i++)*p++=(u16)c;}}
static void line_(int x,int y,int xx,int yy,int c){int dx=absolute(xx-x),dy=-absolute(yy-y),sx=x<xx?1:-1,sy=y<yy?1:-1,e=dx+dy;for(int i=0;i<1200;i++){rect(x,y,1,1,c);if(x==xx&&y==yy)break;int ee=2*e;if(ee>=dy){e+=dy;x+=sx;}if(ee<=dx){e+=dx;y+=sy;}}}
/* Original 5x7 font: minimalist pixel glyphs. Each row is a five-bit bitmap. */
static const u8 glyph[][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
{14,17,17,15,1,1,14},
{0,0,0,0,0,0,0},/* space */
{0,4,4,4,4,0,4},/* ! */
{0,0,0,31,0,0,0},/* - */
{0,0,0,0,0,12,12},/* . */
{0,0,0,0,12,12,0},/* : */
{0,1,2,4,8,16,0},/* / */
{14,17,1,2,4,0,4},/* ? */
{0,0,31,0,31,0,0},/* = */
{4,4,4,31,4,4,4},/* + */
{0,0,0,31,0,0,0},/* _ */
};
static int glyphid(int c){if(c>='a'&&c<='z')c-=32;if(c>='A'&&c<='Z')return c-'A';if(c>='0'&&c<='9')return c-'0'+26;switch(c){case ' ':return 36;case '!':return 37;case '-':return 38;case '.':return 39;case ':':return 40;case '/':return 41;case '?':return 42;case '=':return 43;case '+':return 44;case '_':return 45;default:return 42;}}
static void text_(int x,int y,const char*s,int c,int scale){if(scale<1)scale=1;if(scale>3)scale=3;for(int i=0;s&&s[i]&&i<160;i++){int idx=glyphid((u8)s[i]);if(idx<0||idx>45)idx=42;for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(glyph[idx][row]&(1<<(4-col)))rect(x+col*scale,y+row*scale,scale,scale,c);x+=6*scale;if(x>=W-5)break;}}

#ifndef TEST_HOST
/* Linux MIPS o32 raw syscalls; no dynamically linked libraries needed. */
static inline int sys3(int n,int a,int b,int c){register int v0 __asm__("$2")=n;register int a0 __asm__("$4")=a;register int a1 __asm__("$5")=b;register int a2 __asm__("$6")=c;register int a3 __asm__("$7");__asm__ volatile("syscall":"+r"(v0),"=r"(a3):"r"(a0),"r"(a1),"r"(a2):"memory");return a3?-v0:v0;}
#define PTR(x) ((int)(unsigned long)(x))
static int io_open(const char*p,int f){return sys3(4005,PTR(p),f,0644);}
static int io_close(int fd){return sys3(4006,fd,0,0);}
static int io_read(int fd,void*p,int n){return sys3(4003,fd,PTR(p),n);}
static int io_write(int fd,const void*p,int n){return sys3(4004,fd,PTR(p),n);}
static int io_seek(int fd,int off){return sys3(4019,fd,off,0);}
static int io_ioctl(int fd,int n,void*p){return sys3(4054,fd,n,PTR(p));}
static int io_dents(int fd,void*p,int n){return sys3(4219,fd,PTR(p),n);}
static void io_sleep(void){int ts[2]={0,40000000};sys3(4166,PTR(ts),0,0);}
static void io_exit(int x){sys3(4001,x,0,0);for(;;){}}
#else
#endif
static int loadfile(const char*path,char*dest,int cap){
#ifndef TEST_HOST
 int fd=io_open(path,0);if(fd<0)return -1;int done=0;while(done<cap-1){int n=io_read(fd,dest+done,cap-1-done);if(n<=0)break;done+=n;}dest[done]=0;io_close(fd);return done;
#else
 FILE*f=fopen(path,"rb");if(!f)return -1;int n=(int)fread(dest,1,cap-1,f);dest[n]=0;fclose(f);return n;
#endif
}
/* Bounded HTML/inline JS/CSS reader: HTML and basic canvas tag only. */
static int ci(char c){return (c>='A'&&c<='Z')?c+32:c;}
static const char*find_ci(const char*s,const char*needle){for(;*s;s++){int i=0;while(needle[i]&&s[i]&&ci(needle[i])==ci(s[i]))i++;if(!needle[i])return s;}return 0;}
static int attr_int(const char*tag,const char*key,int default_){const char*p=find_ci(tag,key);if(!p)return default_;p+=strlen_(key);while(issp(*p))p++;if(*p!='=')return default_;p++;while(issp(*p)||*p=='"'||*p=='\'')p++;return isdigit(*p)?parse_int(p):default_;}
static void html_extract(void){
 script_len=0;canvas_w=320;canvas_h=240;bg=color565(10,18,33);
 const char *tag=find_ci(html,"<canvas");if(tag){const char*e=tag;while(*e&&*e!='>')e++;char attrs[512];int n=clamp((int)(e-tag),0,511);memcpy(attrs,tag,n);attrs[n]=0;canvas_w=clamp(attr_int(attrs,"width",320),1,320);canvas_h=clamp(attr_int(attrs,"height",240),1,240);}
 const char*s=html;while((s=find_ci(s,"<script"))){const char*a=s;while(*a&&*a!='>')a++;if(!*a)break;a++;const char*b=find_ci(a,"</script");if(!b)break;int n=(int)(b-a);if(n>0&&script_len+n+2<MAX_JS){memcpy(script+script_len,a,n);script_len+=n;script[script_len++]='\n';}s=b+8;}
 script[script_len]=0;
 const char*sty=find_ci(html,"<style");if(sty){while(*sty&&*sty!='>')sty++;if(*sty)sty++;const char*end=find_ci(sty,"</style");if(end){const char*p=find_ci(sty,"background:");if(!p||p>=end)p=find_ci(sty,"background-color:");if(p&&p<end){p+=prefix(p,"background-color:")?17:11;while(issp(*p))p++;char v[32];int k=0;while(*p&&!issp(*p)&&*p!=';'&&*p!='}'&&k<31)v[k++]=*p++;v[k]=0;bg=colorparse(v);}}}
}

/* Deliberately limited JS interpreter: numbers (8-bit fixed point), strings,
 * variables, functions, if/else, boolean and arithmetic expressions.
 * Example: let x=30; function update(){ if(keys.ArrowRight)x+=2; }
 * function draw(){ctx.fillStyle='#fff';ctx.fillRect(x,20,15,15);}
 */
typedef struct {int kind;int n;char s[128];} Value; /* kind 0=number, 1=string, 2=ctx, 3=canvas */
typedef struct {char name[48];Value v;} Var;
typedef struct {char name[48];int start,end;} Func;
static Var vars[MAX_VARS];static Func funcs[MAX_FUNCS];
static int nvars,nfuncs,budget,js_error;
static const char*src;
static int pos,tk,tn;
static char lex[128];
enum {T_END=0,T_IDENT=256,T_NUM,T_STR,T_EQ,T_NE,T_LE,T_GE,T_AND,T_OR,T_PLUS_EQ,T_MINUS_EQ,T_INC,T_DEC};
static void next(void){while(issp(src[pos]))pos++;if(src[pos]=='/'&&src[pos+1]=='/'){while(src[pos]&&src[pos]!='\n')pos++;next();return;}if(src[pos]=='/'&&src[pos+1]=='*'){pos+=2;while(src[pos]&&!(src[pos]=='*'&&src[pos+1]=='/'))pos++;if(src[pos])pos+=2;next();return;}
 char c=src[pos];lex[0]=0;tn=0;if(!c){tk=T_END;return;}
 if(isalpha(c)){int j=0;while(isalpha(src[pos])||isdigit(src[pos])){if(j<127)lex[j++]=src[pos];pos++;}lex[j]=0;tk=T_IDENT;return;}
 if(isdigit(c)){int v=0;while(isdigit(src[pos])){if(v<1000000)v=v*10+src[pos]-'0';pos++;}v*=FIX;if(src[pos]=='.'){pos++;int part=0,mul=FIX/10;while(isdigit(src[pos])){part+=(src[pos++]-'0')*mul;mul/=10;}v+=part;}tn=v;tk=T_NUM;return;}
 if(c=='\''||c=='"'||c=='`'){char quote=c;pos++;int j=0;while(src[pos]&&src[pos]!=quote){char z=src[pos++];if(z=='\\'&&src[pos]){char t=src[pos++];z=t=='n'?'\n':t=='t'?'\t':t;}if(j<127)lex[j++]=z;}if(src[pos]==quote)pos++;lex[j]=0;tk=T_STR;return;}
 pos++;if(c=='='&&src[pos]=='='){pos++;if(src[pos]=='=')pos++;tk=T_EQ;return;}if(c=='!'&&src[pos]=='='){pos++;if(src[pos]=='=')pos++;tk=T_NE;return;}if(c=='<'&&src[pos]=='='){pos++;tk=T_LE;return;}if(c=='>'&&src[pos]=='='){pos++;tk=T_GE;return;}if(c=='&'&&src[pos]=='&'){pos++;tk=T_AND;return;}if(c=='|'&&src[pos]=='|'){pos++;tk=T_OR;return;}if(c=='+'&&src[pos]=='='){pos++;tk=T_PLUS_EQ;return;}if(c=='-'&&src[pos]=='='){pos++;tk=T_MINUS_EQ;return;}if(c=='+'&&src[pos]=='+'){pos++;tk=T_INC;return;}if(c=='-'&&src[pos]=='-'){pos++;tk=T_DEC;return;}tk=(u8)c;}
static Value num(int n){Value v;memset(&v,0,sizeof(v));v.n=n;return v;}
static Value stringval(const char*s){Value v;memset(&v,0,sizeof(v));v.kind=1;strcopy(v.s,s,128);return v;}
static int truth(Value v){return v.kind==1?v.s[0]!=0:v.n!=0;}
static Var*lookup(const char*name,int create){for(int i=0;i<nvars;i++)if(eq(vars[i].name,name))return &vars[i];if(!create||nvars>=MAX_VARS)return 0;Var*v=&vars[nvars++];memset(v,0,sizeof(*v));strcopy(v->name,name,48);return v;}
static int key_name(const char*s){if(eq(s,"ArrowLeft")||eq(s,"left"))return 0;if(eq(s,"ArrowRight")||eq(s,"right"))return 1;if(eq(s,"ArrowUp")||eq(s,"up"))return 2;if(eq(s,"ArrowDown")||eq(s,"down"))return 3;if(eq(s,"Space")||eq(s," ")||eq(s,"z")||eq(s,"Z"))return 4;if(eq(s,"x")||eq(s,"X"))return 5;if(eq(s,"Enter"))return 6;if(eq(s,"Escape"))return 7;if(eq(s,"q")||eq(s,"Q"))return 8;if(eq(s,"e")||eq(s,"E"))return 9;return -1;}
static int rgb_math(int a,int b,int op){if(op=='*')return ((a/FIX)*b)+((a%FIX)*b)/FIX;if(op=='/')return b?(a/(b/FIX?b/FIX:1)):0;if(op=='%')return b?a%b:0;return 0;}
static int randomseed=87321;static int rand32(void){randomseed=(int)((u32)randomseed*1664525u+1013904223u);return (int)((u32)randomseed>>9);}
static Value builtin(const char*name,Value*a,int n,int active){if(!active)return num(0);int x=n?a[0].n:0,y=n>1?a[1].n:0;if(eq(name,"Math.random"))return num(rand32()%FIX);if(eq(name,"Math.floor"))return num((x/FIX)*FIX);if(eq(name,"Math.ceil"))return num(((x+FIX-1)/FIX)*FIX);if(eq(name,"Math.round"))return num(((x+FIX/2)/FIX)*FIX);if(eq(name,"Math.abs"))return num(absolute(x));if(eq(name,"Math.max"))return num(x>y?x:y);if(eq(name,"Math.min"))return num(x<y?x:y);if(eq(name,"Math.sign"))return num(x>0?FIX:x<0?-FIX:0);
 if(eq(name,"ctx.fillRect")&&n>=4){rect(a[0].n/FIX,a[1].n/FIX,a[2].n/FIX,a[3].n/FIX,fill_color);return num(0);}
 if(eq(name,"ctx.clearRect")&&n>=4){rect(a[0].n/FIX,a[1].n/FIX,a[2].n/FIX,a[3].n/FIX,bg);return num(0);}
 if(eq(name,"ctx.strokeRect")&&n>=4){int xx=a[0].n/FIX,yy=a[1].n/FIX,ww=a[2].n/FIX,hh=a[3].n/FIX;rect(xx,yy,ww,1,stroke_color);rect(xx,yy+hh-1,ww,1,stroke_color);rect(xx,yy,1,hh,stroke_color);rect(xx+ww-1,yy,1,hh,stroke_color);return num(0);}
 if(eq(name,"ctx.fillText")&&n>=3){char t[128];if(a[0].kind==1)strcopy(t,a[0].s,128);else{int z=a[0].n/FIX,k=0;if(z<0){t[k++]='-';z=-z;}char b[16];int j=0;do{b[j++]=(char)('0'+z%10);z/=10;}while(z&&j<15);while(j)t[k++]=b[--j];t[k]=0;}text_(a[1].n/FIX,a[2].n/FIX,t,fill_color,1);return num(0);}
 if(eq(name,"console.log"))return num(0);
 if(eq(name,"document.getElementById")){Value v=num(0);v.kind=3;return v;}
 if(eq(name,"canvas.getContext")){Value v=num(0);v.kind=2;return v;}
 if(eq(name,"ctx.beginPath")||eq(name,"ctx.closePath")||eq(name,"ctx.save")||eq(name,"ctx.restore"))return num(0);
 return num(0);
}
static Value expression(int minp,int active);
static Value atom(int active){Value v=num(0);if(tk=='!'||tk=='-'||tk=='+'){int op=tk;next();v=atom(active);if(op=='!')v=num(!truth(v)?FIX:0);if(op=='-')v.n=-v.n;return v;}
 if(tk==T_NUM){v=num(tn);next();return v;}if(tk==T_STR){v=stringval(lex);next();return v;}
 if(tk=='('){next();v=expression(0,active);if(tk==')')next();return v;}
 if(tk==T_IDENT){char name[128];strcopy(name,lex,128);next();while(tk=='.'){next();if(tk==T_IDENT){int k=strlen_(name),j=0;if(k<123){name[k++]='.';while(lex[j]&&k<127)name[k++]=lex[j++];name[k]=0;}next();}}
 if(tk=='('){Value args[10];int n=0;next();int guard=0;while(tk!=T_END&&tk!=')'&&guard++<15){Value a=expression(0,active);if(n<10)args[n++]=a;if(tk!=',')break;next();}if(tk==')')next();v=builtin(name,args,n,active);return v;}
 if(eq(name,"true"))return num(FIX);if(eq(name,"false")||eq(name,"null"))return num(0);
 if(prefix(name,"keys.")||prefix(name,"keyboard.")){int k=key_name(name+(name[0]=='k'?5:9));return num(k>=0&&keys[k]?FIX:0);}
 if(eq(name,"canvas.width"))return num(canvas_w*FIX);if(eq(name,"canvas.height"))return num(canvas_h*FIX);
 Var*p=lookup(name,0);if(p)return p->v;return num(0);
 }
 next();return v;
}
static int precedence(int t){if(t==T_OR)return 1;if(t==T_AND)return 2;if(t==T_EQ||t==T_NE)return 3;if(t=='<'||t=='>'||t==T_LE||t==T_GE)return 4;if(t=='+'||t=='-')return 5;if(t=='*'||t=='/'||t=='%')return 6;return -1;}
static Value expression(int minp,int active){Value v=atom(active);for(;;){int p=precedence(tk);if(p<minp)break;int op=tk;next();Value b=expression(p+1,active);if(!active)continue;int a=v.n;switch(op){case '+':if(v.kind==1||b.kind==1){char z[128];char t[30];if(v.kind==1)strcopy(z,v.s,128);else{int xx=a/FIX,k=0;if(xx<0){t[k++]='-';xx=-xx;}char rev[12];int m=0;do{rev[m++]=(char)('0'+xx%10);xx/=10;}while(xx&&m<11);while(m)t[k++]=rev[--m];t[k]=0;strcopy(z,t,128);}int k=strlen_(z);if(b.kind==1){strcopy(z+k,b.s,128-k);}else{int xx=b.n/FIX,j=0;char tmp[20];do{tmp[j++]=(char)('0'+xx%10);xx/=10;}while(xx&&j<19);while(j&&k<127)z[k++]=tmp[--j];z[k]=0;}v=stringval(z);}else v=num(a+b.n);break;
 case '-':v=num(a-b.n);break;case '*':v=num(rgb_math(a,b.n,'*'));break;case '/':v=num(b.n?((a/FIX)*FIX/(b.n/FIX?b.n/FIX:1)):0);break;case '%':v=num(b.n?a%b.n:0);break;case T_EQ:v=num(a==b.n?FIX:0);break;case T_NE:v=num(a!=b.n?FIX:0);break;case '<':v=num(a<b.n?FIX:0);break;case '>':v=num(a>b.n?FIX:0);break;case T_LE:v=num(a<=b.n?FIX:0);break;case T_GE:v=num(a>=b.n?FIX:0);break;case T_AND:v=num(truth(v)&&truth(b)?FIX:0);break;case T_OR:v=num(truth(v)||truth(b)?FIX:0);break;default:break;}}
 return v;}
static void statement(int active,int depth);
static void block(int active,int depth){if(depth>24){js_error=1;return;}if(tk=='{'){next();while(tk!=T_END&&tk!='}'&&budget-->0&&!js_error)statement(active,depth+1);if(tk=='}')next();}else statement(active,depth+1);}
static void statement(int active,int depth){if(tk==T_END)return;if(tk==';'){next();return;}if(tk=='{'){block(active,depth);return;}if(tk!=T_IDENT){next();return;}
 if(eq(lex,"if")){next();if(tk=='(')next();Value cond=expression(0,active);if(tk==')')next();block(active&&truth(cond),depth);if(tk==T_IDENT&&eq(lex,"else")){next();block(active&&!truth(cond),depth);}return;}
 if(eq(lex,"function")){next();char name[48]={0};if(tk==T_IDENT){strcopy(name,lex,48);next();}if(tk=='('){int level=1;next();while(tk!=T_END&&level){if(tk=='(')level++;if(tk==')')level--;next();}}if(tk=='{'){int start=pos;int n=1;while(src[pos]&&n){if(src[pos]=='{')n++;if(src[pos]=='}')n--;pos++;}if(nfuncs<MAX_FUNCS){Func*f=&funcs[nfuncs++];strcopy(f->name,name,48);f->start=start;f->end=pos-1;}next();}return;}
 int decl=eq(lex,"let")||eq(lex,"var")||eq(lex,"const");if(decl){next();if(tk!=T_IDENT)return;}
 char name[128];strcopy(name,lex,128);next();while(tk=='.'){next();if(tk==T_IDENT){int k=strlen_(name);if(k<120){name[k++]='.';strcopy(name+k,lex,128-k);}next();}}
 if(tk=='='||tk==T_PLUS_EQ||tk==T_MINUS_EQ||tk==T_INC||tk==T_DEC){int op=tk;next();Value b=op==T_INC||op==T_DEC?num(FIX):expression(0,active);if(active){if(eq(name,"ctx.fillStyle"))fill_color=colorparse(b.kind==1?b.s:"");else if(eq(name,"ctx.strokeStyle"))stroke_color=colorparse(b.kind==1?b.s:"");else{Var*p=lookup(name,1);if(p){if(op=='=')p->v=b;else if(op==T_PLUS_EQ)p->v.n+=b.n;else if(op==T_MINUS_EQ)p->v.n-=b.n;else if(op==T_INC)p->v.n+=FIX;else p->v.n-=FIX;}}}}
 else if(tk=='('){Value a[10];int n=0;next();for(int limit=0;tk!=T_END&&tk!=')'&&limit<12;limit++){Value v=expression(0,active);if(n<10)a[n++]=v;if(tk!=',')break;next();}if(tk==')')next();builtin(name,a,n,active);}
 if(tk==';')next();}
static void js_exec(const char*s){src=s;pos=0;budget=MAX_INSTRUCTIONS;js_error=0;next();while(tk!=T_END&&budget-->0&&!js_error)statement(1,0);if(budget<=0)js_error=1;}
static void runfunc(const char*name){for(int i=0;i<nfuncs;i++)if(eq(funcs[i].name,name)){const char*old=src;char saved=script[funcs[i].end];script[funcs[i].end]=0;src=script;pos=funcs[i].start;budget=MAX_INSTRUCTIONS;js_error=0;next();while(tk!=T_END&&budget-->0&&!js_error)statement(1,0);script[funcs[i].end]=saved;src=old;return;}}
static void gameinit(void){nvars=nfuncs=frames=0;memset(vars,0,sizeof(vars));memset(funcs,0,sizeof(funcs));memset(keys,0,sizeof(keys));fill_color=stroke_color=0xffff;html_extract();js_exec(script);}
static void stepgame(void){rect(0,0,W,H,bg);runfunc("update");runfunc("draw");if(js_error){text_(4,220,"SCRIPT ERROR",color565(255,40,40),1);}frames++;}
static void menu(void){rect(0,0,W,H,color565(12,18,32));rect(0,0,W,27,color565(20,63,91));text_(10,9,"HTMLPLAY RS97",0xffff,1);text_(12,38,"TUS JUEGOS HTML",color565(167,208,255),1);
 if(!game_count)text_(12,73,"NO HAY .HTML",0xffff,1);
 int offset=selection>=12?selection-11:0;for(int i=offset;i<game_count&&i<offset+12;i++){int y=66+(i-offset)*12;if(i==selection){rect(6,y-2,309,11,color565(49,93,126));text_(12,y,games[i],0xffff,1);}else text_(12,y,games[i],color565(168,181,201),1);}
 rect(0,216,320,24,color565(24,39,61));text_(8,222,"ARRIBA/ABAJO  A:ABRIR",0xffff,1);}
static void add_game(const char*s){if(game_count>=MAX_FILES||strlen_(s)>77||!ends(s,".html"))return;for(int i=0;i<game_count;i++)if(eq(games[i],s))return;strcopy(games[game_count++],s,80);}
static int read_game(int idx){char path[256]="/home/retrofw/apps/htmlplay/games/";int k=strlen_(path);strcopy(path+k,games[idx],256-k);if(loadfile(path,html,sizeof(html))<0)return 0;gameinit();return 1;}
#ifndef TEST_HOST
static int fbfd=-1,fd[8],nd;
struct fixinfo{char id[16];u32 smem_start,smem_len,type,type_aux,visual;u16 xpanstep,ypanstep,ywrapstep;u16 pad;u32 line_length;u32 mmio_start,mmio_len,accel,capabilities;};
struct bitf{u32 offset,length,msb_right;};
struct varinfo{u32 xres,yres,xres_virtual,yres_virtual,xoffset,yoffset,bits_per_pixel,grayscale;struct bitf red,green,blue,transp;u32 nonstd,activate,height,width,accel_flags,pixclock,left_margin,right_margin,upper_margin,lower_margin,hsync_len,vsync_len,sync,vmode,rotate,colorspace,reserved[4];};
static void scan_files(void){int d=io_open("/home/retrofw/apps/htmlplay/games",0);if(d<0)return;char buf[2048];int n;while((n=io_dents(d,buf,sizeof(buf)))>0){int off=0;while(off+19<n){u16 reclen=(u8)buf[off+16]|((u8)buf[off+17]<<8);if(reclen<20||off+reclen>n)break;add_game(buf+off+19);off+=reclen;}}io_close(d);}
static void frame(void){io_seek(fbfd,0);const u8*p=(const u8*)fb;int left=sizeof(fb);while(left>0){int n=io_write(fbfd,p,left);if(n<=0)break;p+=n;left-=n;}}
static int keycode(int code){switch(code){case 105:return 0;case 106:return 1;case 103:return 2;case 108:return 3;case 57:case 30:case 304:return 4;case 45:case 31:case 305:return 5;case 28:return 6;case 1:case 14:case 54:return 7;case 16:case 310:return 8;case 18:case 311:return 9;default:return -1;}}
static void poll(void){for(int i=0;i<nd;i++){u8 e[16];while(io_read(fd[i],e,16)==16){u16 type=(u16)e[8]|((u16)e[9]<<8),code=(u16)e[10]|((u16)e[11]<<8);int val=(int)((u32)e[12]|((u32)e[13]<<8)|((u32)e[14]<<16)|((u32)e[15]<<24));if(type!=1||code>=512)continue;int prior=down[code];down[code]=val!=0;int k=keycode(code);if(k>=0)keys[k]=val!=0;if(prior||val!=1)continue;
 if(in_game){if(k==7){in_game=0;memset(keys,0,sizeof(keys));}else if(k==6&&down[54])in_game=0;}else{if(k==2&&selection>0)selection--;if(k==3&&selection<game_count-1)selection++;if((k==4||k==6)&&game_count&&read_game(selection))in_game=1;if(k==7)running=0;}}
 }}
static void app(void){fbfd=io_open("/dev/fb0",2);if(fbfd<0)return;struct varinfo vi;struct fixinfo fi;memset(&vi,0,sizeof(vi));memset(&fi,0,sizeof(fi));if(io_ioctl(fbfd,0x4600,&vi)<0||vi.xres!=W||vi.yres!=H||vi.bits_per_pixel!=16||io_ioctl(fbfd,0x4602,&fi)<0||fi.line_length!=W*2){io_close(fbfd);return;}
 for(int i=0;i<8;i++){char p[]="/dev/input/event0";p[16]='0'+i;int f=io_open(p,0x80);if(f>=0)fd[nd++]=f;}
 scan_files();while(running){poll();if(in_game)stepgame();else menu();frame();io_sleep();}for(int i=0;i<nd;i++)io_close(fd[i]);io_close(fbfd);}
void _start(void){app();io_exit(0);}
#else
int main(int argc,char**argv){if(argc<3){fprintf(stderr,"Usage: %s example.html output.ppm [frames]\n",argv[0]);return 1;}if(loadfile(argv[1],html,sizeof(html))<0)return 2;gameinit();if(argc>3)keys[1]=1;for(int i=0;i<5;i++)stepgame();FILE*f=fopen(argv[2],"wb");if(!f)return 3;fprintf(f,"P6\n320 240\n255\n");for(int i=0;i<W*H;i++){u16 c=fb[i];u8 rgb[3]={(u8)(((c>>11)&31)*255/31),(u8)(((c>>5)&63)*255/63),(u8)((c&31)*255/31)};fwrite(rgb,1,3,f);}fclose(f);fprintf(stderr,"vars:%d funcs:%d frames:%d js_error:%d scriptsize:%d\n",nvars,nfuncs,frames,js_error,script_len);return js_error?5:0;}
#endif
