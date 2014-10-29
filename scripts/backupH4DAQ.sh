#!/bin/bash

input_dir=/media/backup_h4_data_2
output_dir=/t3/users/cmsdaqtb/data/data/H4Fall2014/
id=`date '+%Y%m%d_%H%M%S'`

cd ${input_dir}

for file in `find . -type f -regex ".*\(root\|raw\)" | sort`; do rsync -R -avze "ssh -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --log-file=/tmp/${USER}/backup_${id} --progress $file meridian@cmsrm-an.roma1.infn.it:${output_dir}; done
