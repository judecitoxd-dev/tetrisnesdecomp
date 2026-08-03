#!/usr/bin/env python3
"""v0.24 supplemental PRG verifier. It changes no game/runtime files."""
from __future__ import annotations
import argparse,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def load_base():
 s=importlib.util.spec_from_file_location('base',HERE/'prg_verify.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def n(v): return v if isinstance(v,int) else int(v,0)
def be(b): return [(b[i]<<8)|b[i+1] for i in range(0,len(b),2)]
def le(b): return [b[i]|(b[i+1]<<8) for i in range(0,len(b),2)]
def union_bytes(items):
 r=sorted((n(x['address']),n(x['address'])+x['length']) for x in items); total=0
 if not r:return 0
 a,z=r[0]
 for x,y in r[1:]:
  if x<=z:z=max(z,y)
  else:total+=z-a;a,z=x,y
 return total+z-a

def run(rom,base_path,extra_path):
 b=load_base(); base=json.loads(base_path.read_text()); extra=json.loads(extra_path.read_text()); merged=json.loads(json.dumps(base))
 merged['routine_signatures']+=extra['routine_ranges']; merged['tables']+=extra['tables']
 data=rom.read_bytes(); report=b.verify_rom_bytes(data,merged); prg,_=b.parse_ines(data)
 t={x['name']:b.slice_at(prg,n(x['address']),x['length']) for x in extra['tables']}; e=extra['semantic_checks']; errors=[]; passed=0
 def ok(name,actual,wanted):
  nonlocal passed
  if list(actual)!=list(wanted): errors.append('semantic '+name+': values differ')
  else: passed+=1
 for k in ('rotation_table','music_selection_table','level_cursor_y','level_cursor_x','height_cursor_y','height_cursor_x','type_b_blank_count_by_height','type_b_rng_tile_table','orientation_to_sprite_table','note_duration_table_ntsc','music_data_table_index'):ok(k,t[k],e[k])
 ok('piece_to_ppu_stat_addresses',be(t['piece_to_ppu_stat_addr']),e['piece_to_ppu_stat_addresses'])
 ok('vram_playfield_row_addresses',le(t['vram_playfield_rows']),e['vram_playfield_row_addresses'])
 ok('high_score_ppu_addresses',be(t['high_score_ppu_addresses']),e['high_score_ppu_addresses'])
 ok('level_display_values',[((x>>4)*10+(x&15)) for x in t['level_display_bcd']],e['level_display_values'])
 ok('mult_by_10_values',t['mult_by_10_table'],e['mult_by_10_values'])
 ok('byte_to_bcd_0_49',t['byte_to_bcd_0_49'],[((i//10)<<4)|(i%10) for i in range(50)])
 ok('high_score_name_offsets',t['high_score_offset_tables'][:8],e['high_score_name_offsets']);ok('high_score_score_offsets',t['high_score_offset_tables'][8:],e['high_score_score_offsets'])
 ok('high_score_cursor_y',t['high_score_cursor_tables'][:3],e['high_score_cursor_y']);ok('high_score_cursor_x',t['high_score_cursor_tables'][3:],e['high_score_cursor_x'])
 c=t['high_score_character_tiles']; ok('high_score_character_tiles',[len(c),c[-1],len(set(c[:-1]))],[44,255,43])
 p=le(t['sound_effect_dispatch_tables']);ok('sound_effect_dispatch_tables',[len(p),all(0xE000<=x<=0xFFFF for x in p)],[e['sound_effect_dispatch_pointer_count'],True])
 p=le(t['music_volume_pointer_table']);ok('music_volume_pointer_table',[len(p),all(0xEA76<=x<0xEB13 for x in p)],[e['music_volume_pointer_count'],True])
 h=t['music_track_headers']; no=[];du=[];ptr=True
 for i in range(10):
  x=h[i*10:(i+1)*10];no.append(x[0]);du.append(x[1]);ptr&=all(y==0xFFFF or 0x8000<=y<=0xFFFF for y in le(x[2:]))
 ok('music_track_headers',[no,du,ptr],[e['music_track_note_offsets'],e['music_track_duration_offsets'],True])
 timers=be(t['note_to_wave_table']);ok('note_to_wave_table',[len(timers),timers[0],timers[1],timers[26],timers[35]],[78,0x07F0,0,0x01AB,0x00FD])
 report['errors']+=errors;report['ok']=not report['errors'];report['v024_semantic_checks_verified']=passed
 report['unique_verified_prg_bytes']=union_bytes(merged['routine_signatures']+merged['tables']);report['runtime_files_changed']=0
 return report

def main():
 p=argparse.ArgumentParser();p.add_argument('rom',nargs='?',type=Path);p.add_argument('--base',type=Path,default=HERE/'tetris_prg_manifest.json');p.add_argument('--extra',type=Path,default=HERE/'tetris_prg_manifest_v024.json');p.add_argument('--report',type=Path);p.add_argument('--self-test',action='store_true');a=p.parse_args()
 if a.self_test:
  try:
   x=json.loads(a.extra.read_text());assert x['coverage']['runtime_files_changed']==0 and len(x['routine_ranges'])>=20 and len(x['tables'])>=25 and union_bytes(x['routine_ranges']+x['tables'])>2500
  except Exception as ex: print('prg_verify_v024 self-test failed:',ex,file=sys.stderr);return 1
  print(f"prg_verify_v024 self-test: OK routines={len(x['routine_ranges'])} tables={len(x['tables'])} runtime_changes=0");return 0
 if not a.rom:p.error('ROM path required')
 try:r=run(a.rom,a.base,a.extra)
 except Exception as ex:print('error:',ex,file=sys.stderr);return 1
 if a.report:a.report.write_text(json.dumps(r,indent=2)+'\n')
 if not r['ok']:
  for x in r['errors']:print('FAIL:',x,file=sys.stderr)
  return 1
 print(f"PRG v0.24 verification: OK routines={r['routine_signatures_verified']} tables={r['tables_verified']} edges={r['control_flow_edges_verified']} semantic={r['functional_checks_verified']+r['v024_semantic_checks_verified']}")
 print('unique verified PRG bytes:',r['unique_verified_prg_bytes']);print('runtime files changed: 0');return 0
if __name__=='__main__':raise SystemExit(main())
