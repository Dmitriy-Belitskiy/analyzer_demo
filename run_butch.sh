ls /data/raw_data/*.root | \
parallel -j 16 \
cmsRun python/demo_cfg.py \
inFile=file:{} \
outFile=/data/output_9_cut/output_{/}.root
