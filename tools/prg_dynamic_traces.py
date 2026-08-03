#!/usr/bin/env python3
"""Capture 18 deterministic dynamic PRG trace families from a legal Tetris NES ROM.

The committed reference contains only hashes and recipes. Full RAM/write traces stay local.
This is a research tool and does not modify or execute the native port runtime.
"""
from __future__ import annotations
import argparse, hashlib, json, sys, zlib
from pathlib import Path

C=1; Z=2; I=4; U=0x20; V=0x40; N=0x80
A_BTN=0x80; DOWN=4; RIGHT=1; EMPTY=0xEF
BASE={0x09:2,0x0A:2,0x10:2,0x18:2,0x20:6,0x29:2,0x30:2,0x38:2,0x45:3,0x4A:2,0x4C:3,0x60:6,0x65:3,0x69:2,0x76:6,0x85:3,0x86:3,0x88:2,0x8A:2,0x8D:4,0x90:2,0x91:6,0x95:4,0x9D:5,0xA0:2,0xA2:2,0xA4:3,0xA5:3,0xA6:3,0xA8:2,0xA9:2,0xAA:2,0xB0:2,0xB1:5,0xB5:4,0xB9:4,0xBD:4,0xC0:2,0xC5:3,0xC6:5,0xC8:2,0xC9:2,0xCA:2,0xD0:2,0xE0:2,0xE4:3,0xE6:5,0xE8:2,0xE9:2,0xF0:2}
PAGE={0xB1,0xB9,0xBD}

class CPU:
 def __init__(s,prg,ram):
  s.prg=prg;s.ram=ram;s.a=s.x=s.y=0;s.sp=0xFD;s.p=I|U;s.pc=0;s.cycles=s.ins=0;s.w=[];s.hw=[];s.page=False;s.bx=0
 def rd(s,a):
  a&=0xFFFF
  if a<0x2000:return s.ram[a&0x7FF]
  if a>=0x8000:return s.prg[a-0x8000]
  return 0
 def wr(s,a,v):
  a&=0xFFFF;v&=255
  if a<0x2000:
   a&=0x7FF;o=s.ram[a];s.ram[a]=v
   if o!=v:s.w.append((a,v))
  else:s.hw.append((a,v))
 def f8(s):v=s.rd(s.pc);s.pc=(s.pc+1)&0xFFFF;return v
 def f16(s):return s.f8()|(s.f8()<<8)
 def push(s,v):s.wr(0x100|s.sp,v);s.sp=(s.sp-1)&255
 def pull(s):s.sp=(s.sp+1)&255;return s.rd(0x100|s.sp)
 def nz(s,v):s.p=(s.p&~(N|Z))|(Z if (v&255)==0 else 0)|(v&N)
 def cmp(s,l,r):s.p=(s.p&~C)|(C if l>=r else 0);s.nz((l-r)&255)
 def adc(s,v):
  q=s.a+v+(1 if s.p&C else 0);r=q&255;s.p=(s.p&~(C|V))|(C if q>255 else 0)|(V if (~(s.a^v)&(s.a^r)&128) else 0);s.a=r;s.nz(r)
 def zp(s):return s.f8()
 def zpx(s):return (s.f8()+s.x)&255
 def ab(s):return s.f16()
 def idx(s,b,i):r=(b+i)&0xFFFF;s.page=(b&0xFF00)!=(r&0xFF00);return r
 def ax(s):return s.idx(s.f16(),s.x)
 def ay(s):return s.idx(s.f16(),s.y)
 def i16(s,a):return s.rd(a)|(s.rd((a+1)&255)<<8)
 def iy(s):return s.idx(s.i16(s.f8()),s.y)
 def branch(s,t):
  o=s.f8();o=o-256 if o&128 else o
  if t:
   old=s.pc;s.pc=(s.pc+o)&0xFFFF;s.bx=1+(1 if (old&0xFF00)!=(s.pc&0xFF00) else 0)
 def dec(s,a):v=(s.rd(a)-1)&255;s.wr(a,v);s.nz(v)
 def inc(s,a):v=(s.rd(a)+1)&255;s.wr(a,v);s.nz(v)
 def ror(s,a):
  v=s.rd(a);c=128 if s.p&C else 0;s.p=(s.p&~C)|(C if v&1 else 0);v=(v>>1)|c;s.wr(a,v);s.nz(v)
 def step(s):
  op=s.f8();s.ins+=1;s.page=False;s.bx=0
  if op==0x09:s.a|=s.f8();s.nz(s.a)
  elif op==0x0A:s.p=(s.p&~C)|(C if s.a&128 else 0);s.a=(s.a<<1)&255;s.nz(s.a)
  elif op==0x10:s.branch(not s.p&N)
  elif op==0x18:s.p&=~C
  elif op==0x20:
   a=s.f16();r=(s.pc-1)&0xFFFF;s.push(r>>8);s.push(r);s.pc=a
  elif op==0x29:s.a&=s.f8();s.nz(s.a)
  elif op==0x30:s.branch(bool(s.p&N))
  elif op==0x38:s.p|=C
  elif op==0x45:s.a^=s.rd(s.zp());s.nz(s.a)
  elif op==0x4A:s.p=(s.p&~C)|(C if s.a&1 else 0);s.a>>=1;s.nz(s.a)
  elif op==0x4C:s.pc=s.f16()
  elif op==0x60:s.pc=((s.pull()|(s.pull()<<8))+1)&0xFFFF
  elif op==0x65:s.adc(s.rd(s.zp()))
  elif op==0x69:s.adc(s.f8())
  elif op==0x76:s.ror(s.zpx())
  elif op==0x85:s.wr(s.zp(),s.a)
  elif op==0x86:s.wr(s.zp(),s.x)
  elif op==0x88:s.y=(s.y-1)&255;s.nz(s.y)
  elif op==0x8A:s.a=s.x;s.nz(s.a)
  elif op==0x8D:s.wr(s.ab(),s.a)
  elif op==0x90:s.branch(not s.p&C)
  elif op==0x91:s.wr(s.iy(),s.a)
  elif op==0x95:s.wr(s.zpx(),s.a)
  elif op==0x9D:s.wr(s.ax(),s.a)
  elif op==0xA0:s.y=s.f8();s.nz(s.y)
  elif op==0xA2:s.x=s.f8();s.nz(s.x)
  elif op==0xA4:s.y=s.rd(s.zp());s.nz(s.y)
  elif op==0xA5:s.a=s.rd(s.zp());s.nz(s.a)
  elif op==0xA6:s.x=s.rd(s.zp());s.nz(s.x)
  elif op==0xA8:s.y=s.a;s.nz(s.y)
  elif op==0xA9:s.a=s.f8();s.nz(s.a)
  elif op==0xAA:s.x=s.a;s.nz(s.x)
  elif op==0xB0:s.branch(bool(s.p&C))
  elif op==0xB1:s.a=s.rd(s.iy());s.nz(s.a)
  elif op==0xB5:s.a=s.rd(s.zpx());s.nz(s.a)
  elif op==0xB9:s.a=s.rd(s.ay());s.nz(s.a)
  elif op==0xBD:s.a=s.rd(s.ax());s.nz(s.a)
  elif op==0xC0:s.cmp(s.y,s.f8())
  elif op==0xC5:s.cmp(s.a,s.rd(s.zp()))
  elif op==0xC6:s.dec(s.zp())
  elif op==0xC8:s.y=(s.y+1)&255;s.nz(s.y)
  elif op==0xC9:s.cmp(s.a,s.f8())
  elif op==0xCA:s.x=(s.x-1)&255;s.nz(s.x)
  elif op==0xD0:s.branch(not s.p&Z)
  elif op==0xE0:s.cmp(s.x,s.f8())
  elif op==0xE4:s.cmp(s.x,s.rd(s.zp()))
  elif op==0xE6:s.inc(s.zp())
  elif op==0xE8:s.x=(s.x+1)&255;s.nz(s.x)
  elif op==0xE9:s.adc((~s.f8())&255)
  elif op==0xF0:s.branch(bool(s.p&Z))
  else:raise RuntimeError(f'unsupported opcode {op:02X} at {(s.pc-1)&0xFFFF:04X}')
  s.p|=U;s.cycles+=BASE[op]+s.bx+(1 if s.page and op in PAGE else 0)
 def call(s,a,limit=100000):
  s.pc=a;s.sp=0xFD;entry=s.sp
  for _ in range(limit):
   if s.sp==entry and s.rd(s.pc)==0x60:s.pc=(s.pc+1)&0xFFFF;s.ins+=1;s.cycles+=6;return
   s.step()
  raise RuntimeError(f'instruction limit at {s.pc:04X}')

def rom(path):
 d=path.read_bytes()
 if d[:4]!=b'NES\x1a':raise ValueError('not iNES')
 n=d[4]*16384;o=16+(512 if d[6]&4 else 0);p=d[o:o+n];p=p*2 if len(p)==16384 else p
 if len(p)!=32768:raise ValueError('expected 32 KiB PRG')
 return d,p

def recipes():
 return [
 ('rotation_valid',0x88AB,{'cpu':{'a':0},'ram':{0x40:5,0x41:8,0x42:2,0xB5:A_BTN}}),
 ('gravity_drop',0x8914,{'ram':{0x40:5,0x41:8,0x42:2,0x44:0,0x45:0x30,0x4E:1,0xB5:0,0xB6:0}}),
 ('das_shift_right',0x89AE,{'ram':{0x40:5,0x41:8,0x42:2,0x46:0,0xB5:RIGHT,0xB6:RIGHT}}),
 ('collision_blocked',0x948B,{'ram':{0x40:5,0x41:8,0x42:2},'blocks':[(8,5,0x7B)]}),
 ('spawn_piece',0x988E,{'ram':{0xBF:7,0xBE:1,0xC0:5,0xD3:0}}),
 ('select_demo_piece',0x98EB,{'ram':{0xC0:5,0xD3:0}}),
 ('random_piece',0x9907,{'ram':{0x17:0x89,0x18:0x12,0x19:2,0x1A:4}}),
 ('piece_statistics',0x9969,{'cpu':{'a':2}}),
 ('lock_piece',0x99A2,{'ram':{0x40:5,0x41:8,0x42:2,0x49:0x20,0xC2:0,0xBA:0}}),
 ('game_over_curtain',0x9A11,{'ram':{0x58:0xF0,0xB1:0,0xB2:0}}),
 ('completed_row',0x9A6B,{'ram':{0x40:5,0x41:10,0x42:2,0x49:0x20,0x57:0},'rows':[8]}),
 ('garbage_receive',0x9B03,{'ram':{0xBE:2,0xBB:2,0x49:0x20,0x5A:4},'pattern':1}),
 ('score_and_level',0x9B58,{'ram':{0x56:4,0x44:0,0x50:0,0x51:0,0x53:0,0x54:0,0x55:0,0x48:8,0xC1:0}}),
 ('lock_height',0x9CAF,{'ram':{0x41:7,0x49:0x20}}),
 ('allegro_switch',0x9D17,{'ram':{0xBA:0,0xC2:0},'rows':[5]}),
 ('demo_pointer_advance',0x9DE8,{'ram':{0xD1:0xFE,0xD2:0x8A,0xD3:0x10}}),
 ('music_track_select',0x9E07,{'cpu':{'a':3},'ram':{0x6F5:0,0x6FD:0}}),
 ('rng_lfsr',0xAB47,{'cpu':{'x':0x17,'y':2},'ram':{0x17:0x89,0x18:0x12}})]

def run(prg,name,entry,cfg):
 r=bytearray(0x800);r[0x400:0x4C8]=bytes([EMPTY])*200;r[0xB8]=0;r[0xB9]=4
 for a,v in cfg.get('ram',{}).items():r[a]=v&255
 for row in cfg.get('rows',[]):r[0x400+row*10:0x40A+row*10]=bytes([0x7B])*10
 for y,x,v in cfg.get('blocks',[]):r[0x400+y*10+x]=v
 if cfg.get('pattern'):
  for i in range(200):r[0x400+i]=(i%7)+0x70
 before=bytes(r);c=CPU(prg,r)
 for k,v in cfg.get('cpu',{}).items():setattr(c,k,v)
 c.call(entry)
 changed=[{'address':f'{i:04X}','before':before[i],'after':r[i]} for i in range(0x800) if before[i]!=r[i] and not 0x100<=i<0x200]
 t={'family':name,'entry':f'{entry:04X}','instructions':c.ins,'cycles':c.cycles,'registers':{'a':c.a,'x':c.x,'y':c.y,'sp':c.sp,'p':c.p,'pc':c.pc},'ram_changes':changed,'hardware_writes':[{'address':f'{a:04X}','value':v} for a,v in c.hw]}
 t['sha256']=hashlib.sha256(json.dumps(t,sort_keys=True,separators=(',',':')).encode()).hexdigest();return t

def selftest():
 p=bytearray([0xA9,0x2A,0x85,0x10,0xE6,0x10,0x60])+bytearray([0xEA])*(32768-7);r=bytearray(0x800);c=CPU(bytes(p),r);c.call(0x8000)
 assert r[0x10]==0x2B and c.a==0x2A and c.ins==4 and len(recipes())==18
 print('prg_dynamic_traces self-test: OK families=18')

def main():
 ap=argparse.ArgumentParser();ap.add_argument('rom',nargs='?',type=Path);ap.add_argument('--output',type=Path);ap.add_argument('--summary',type=Path);ap.add_argument('--expect',type=Path);ap.add_argument('--self-test',action='store_true');a=ap.parse_args()
 if a.self_test:selftest();return 0
 if not a.rom or not a.output:ap.error('ROM and --output required')
 d,p=rom(a.rom)
 if zlib.crc32(d)&0xffffffff!=0xD16EA396:raise SystemExit('wrong ROM CRC32')
 ts=[]
 for n,e,cfg in recipes():
  t=run(p,n,e,cfg);ts.append(t);print('OK',n,t['instructions'],t['cycles'],t['sha256'][:12])
 full={'schema':1,'rom_crc32':'D16EA396','prg_sha256':hashlib.sha256(p).hexdigest(),'families_completed':18,'families_total':18,'traces':ts};a.output.write_text(json.dumps(full,indent=2)+'\n')
 if a.summary:
  s={'schema':1,'rom_crc32':'D16EA396','prg_sha256':full['prg_sha256'],'families_completed':18,'families_total':18,'trace_hashes':{t['family']:t['sha256'] for t in ts},'aggregate_sha256':hashlib.sha256(''.join(t['sha256'] for t in ts).encode()).hexdigest()};a.summary.write_text(json.dumps(s,indent=2)+'\n')
  if a.expect:
   x=json.loads(a.expect.read_text());keys=('rom_crc32','prg_sha256','families_completed','families_total','trace_hashes','aggregate_sha256')
   bad=[k for k in keys if x.get(k)!=s.get(k)]
   if bad:raise SystemExit('G6 reference mismatch: '+', '.join(bad))
   print('G6_REFERENCE_MATCH=YES')
 elif a.expect:raise SystemExit('--expect requires --summary')
 return 0
if __name__=='__main__':raise SystemExit(main())
