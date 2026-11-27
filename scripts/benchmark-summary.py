#!/usr/bin/env python3
"""Summarize complete native synthetic-index benchmark logs; reject partial runs."""
import argparse,json,re
from pathlib import Path

def read_benchmark(path):
    text=Path(path).read_text();values={}
    for line in text.splitlines():
        if line.startswith('BENCH '):
            values.update({k:int(v) for k,v in re.findall(r'([a-z_0-9]+)=(\d+)',line)})
    required={'items','released_limit','setup_us','synthetic_build_us','cold_ready_us','index_bytes',
              'queries','search_p50_us','search_p95_us','search_max_us','input_p50_us','input_p95_us',
              'input_max_us','render_p95_us','resident_delta_bytes','largest_free_block',
              'cache_load_us','cached_ready_us','restored_items','samples'}
    if not required<=values.keys() or not re.search(r'^BENCH PASS items=\d+ samples=25$',text,re.M):
        raise ValueError('Incomplete or failed benchmark: '+str(path))
    if values['items']!=values['restored_items'] or values['samples']!=25 or values['queries']!=25:
        raise ValueError('Inconsistent benchmark: '+str(path))
    for prefix in ('search','input'):
        if not values[prefix+'_p50_us']<=values[prefix+'_p95_us']<=values[prefix+'_max_us']:
            raise ValueError('Invalid latency percentiles: '+str(path))
    return values

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('logs',nargs='+',type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    rows=sorted((read_benchmark(p) for p in args.logs),key=lambda r:r['items'])
    if [r['items'] for r in rows]!=[2000,10000,50000]:raise ValueError('Require exactly the 2000, 10000 and 50000 fixtures')
    args.output.write_text(json.dumps({'scope':'Synthetic index readiness and search+render timings, not process startup or physical key-to-display latency',
        'cache_storage':'Guest RAM temporary files','workloads':rows},indent=2)+'\n')
    print('PASS: all three benchmark workloads summarized')
if __name__=='__main__':main()
