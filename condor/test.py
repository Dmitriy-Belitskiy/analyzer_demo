import os
import subprocess

# Configuration
run = 392526
input_txt = f"run_list/{run}.txt"  # Input list of ROOT files
submit_dir = "condor_submissions"  # Directory to store submit files
executable_script = "run_cmssw.sh"  # Executable script for CMSSW jobs
output_base = f'/eos/home-d/dmbelyts/root_out3/{run}/'  # EOS output
log_dir = "logs"

# Ensure base directories exist
os.makedirs(submit_dir, exist_ok=True)
os.makedirs(log_dir, exist_ok=True)
os.makedirs(output_base, exist_ok=True)

# DAS query to get file list
os.makedirs("run_list", exist_ok=True)
run_query = f'dasgoclient -query="file dataset=/L1Scouting/Run2025C-v1/L1SCOUT run={run}" | sed "s|^|root://cms-xrd-global.cern.ch/|" > run_list/{run}.txt'
subprocess.run(run_query, shell=True, check=True)

# Read the file list
with open(input_txt, "r") as f:
    files = [line.strip() for line in f if line.strip()]

submit_files = []
files_per_dir = 50

for i, root_file in enumerate(files):
    job_name = f"job_{i}"
    subdir_index = i // files_per_dir
    sub_output_dir = os.path.join(output_base, f"part_{subdir_index}")
    os.makedirs(sub_output_dir, exist_ok=True)

    output_file = os.path.join(sub_output_dir, f"{run}_{i}.root")
    submit_file = os.path.join(submit_dir, f"{job_name}.sub")

    submit_content = f"""universe   = vanilla
executable = {executable_script}
arguments  = {root_file} {output_file}
output     = {log_dir}/{job_name}.out
error      = {log_dir}/{job_name}.err
log        = {log_dir}/{job_name}.log
+JobFlavour = "longlunch"

+AccountingGroup = "group_u_CMS.CAF.COMM"

transfer_input_files = /afs/cern.ch/user/d/dmbelyts/CMSSW_15_0_6/src/Demo/analyzer_demo/python/demo_cfg.py


use_x509userproxy = true
x509userproxy = /afs/cern.ch/user/d/dmbelyts/CMSSW_15_0_6/src/Demo/analyzer_demo/proxy.pem

should_transfer_files = YES
when_to_transfer_output = ON_EXIT

queue 1
"""
    with open(submit_file, "w") as f:
        f.write(submit_content)

    submit_files.append(submit_file)

# Submit jobs and remove submit files if successful
for submit_file in submit_files:
    result = subprocess.run(["condor_submit", submit_file], capture_output=True, text=True)
    if result.returncode == 0:
        print(f"Submitted: {submit_file}")
        os.remove(submit_file)
    else:
        print(f"Failed to submit: {submit_file}\n{result.stderr}")
