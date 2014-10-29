#!/bin/bash

input_dir=""
output_dir=""
id="$$_`date '+%Y%m%d'`"
ssh=0
verbose=0
dryrun=0

TEMP=`getopt -o sdvi:o: --long input:,output:,dryrun,verbose,ssh -n 'localBackup.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Options are wrong..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

while true; do
  case "$1" in
    -v | --verbose ) verbose=1; shift ;;
    -d | --dryrun ) dryrun=1; shift;;
    -s | --ssh ) ssh=1; shift;;
    -i | --input )
      input_dir="$2"; shift 2 ;;
    -o | --output )
      output_dir="$2"; shift 2 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

#-R preserve folder tree structure
rsync_options="-aRvz --log-file=/tmp/${USER}/h4daq_backup_${id}.log --progress"
rsync_ssh="-e \"ssh -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null\""

[ "${dryrun}" == 0 ] || rsync_options="${rsync_options} --dry-run"
[ "${ssh}" == 0 ] || rsync_options="${rsync_options} ${rsync_ssh}"

echo "====> H4DAQ LOCAL BACKUP"
echo "Start copying files from ${input_dir} to ${output_dir} with options ${rsync_options}"

cd ${input_dir}
#Only copy root or raw files keeping the relative folder tree structure
find . -type f -regex ".*\(root\|raw\)" > /tmp/${USER}/h4daq_backup_${id}.tocopy
touch /tmp/${USER}/h4daq_backup_${id}.copied
for file in `cat /tmp/${USER}/h4daq_backup_${id}.tocopy`; do rsync ${rsync_options} ${file} ${output_dir}; [ $? -ne 0 ] || echo "${input_dir}/$file" >> /tmp/${USER}/h4daq_backup_${id}.copied; done

#remove duplicated lines
sort /tmp/${USER}/h4daq_backup_${id}.copied > /tmp/${USER}/h4daq_backup_${id}.copied.sorted
uniq /tmp/${USER}/h4daq_backup_${id}.copied.sorted /tmp/${USER}/h4daq_backup_${id}.copied.uniq
mv -f /tmp/${USER}/h4daq_backup_${id}.copied.uniq /tmp/${USER}/h4daq_backup_${id}.copied
rm -rf /tmp/${USER}/h4daq_backup_${id}.copied.sorted 
