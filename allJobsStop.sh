# GATHER THE JOB_IDS FROM THE POLL RUNNING/QUEUED OUTPUTS 
# USE INTERNAL FIELD SEPARATOR TO READ THE JOB IDS INTO AN ARRAY (running_jobs, queued_jobs), USE AWK TO POLL AND EXTRACT THE JOB IDS.
IFS=$'\n' read -d '' -r -a running_jobs < <(./jobCommander poll running | awk -F' ' '/Job ID/ {sub(/,$/,"",$3); print $3}') 
IFS=$'\n' read -d '' -r -a queued_jobs < <(./jobCommander poll queued | awk -F' ' '/Job ID/ {sub(/,$/,"",$3); print $3}')

# STOP THE QUEUED JOBS FIRST TO AVOID CONFLICT AND EDGE CASES
for job in "${queued_jobs[@]}"; do
    ./jobCommander stop $job
done

# STOP THE RUNNING JOBS AFTER
for job in "${running_jobs[@]}"; do
    ./jobCommander stop $job
done

./jobCommander exit
echo "allJobsStop.sh completed"