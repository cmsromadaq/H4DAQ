#!/bin/bash

dryrun=0
verbosity=3

#start_dr=1
#start_rc=1
#start_eb=0

daquser="cmsdaq"
daqhome="/home/cmsdaq"
norecompile=0
ebrecompile=0
drcvrecompile=0
mybranch="master"
logdir="/tmp"
vmecontroller=1 # 0=V1718(Usb), 1=V2718(Pci)
dr=""
drcv=""
rc=""
eb=""
nice=0
tag=""

TEMP=`getopt -o dv:n: --long nice:,verbose:,logdir:,daquser:,daqhome:,dr:,drcv:,eb:,rc:,vmecontroller:,gitbranch:,tag:,norecompile,ebrecompile,drcvrecompile,dryrun -n 'startall.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Options are wrong..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

while true; do

  case "$1" in
    -v | --verbose ) verbosity="$2"; shift 2 ;;
    -n | --nice ) nice="$2"; shift 2 ;;
    -d | --dryrun ) dryrun=1; shift;;
    --norecompile ) norecompile=1; shift;;
    --ebrecompile ) ebrecompile=1; shift;;
    --drcvrecompile ) drcvrecompile=1; shift;;
    --tag )
      tag="$2"; shift 2 ;;
    --daquser )
      daquser="$2"; shift 2 ;;
    --daqhome )
      daqhome="$2"; shift 2 ;;
    --gitbranch )
      mybranch="$2"; shift 2 ;;
    --vmecontroller )
      vmecontroller="$2"; shift 2 ;;
    --logdir )
      logdir="$2"; shift 2 ;;
    --dr )
      dr="$2"; shift 2 ;;
    --drcv )
      drcv="$2"; shift 2 ;;
    --rc )
      rc="$2"; shift 2 ;;
    --eb )
      eb="$2"; shift 2 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

echo "=================================================================="
echo "Starting H4DAQ installed @${daqhome} as ${daquser}"
[ "${dr}" == "" ] || echo "DR MACHINES: ${dr}"
[ "${rc}" == "" ] || echo "RC MACHINES: ${rc}"
[ "${eb}" == "" ] || echo "EB MACHINES: ${eb}"
echo "=================================================================="

## create repository if does not exists, otherwise update and compile
mycommand="cd ${daqhome};  \
		mkdir -p DAQ ;  \
		cd DAQ ; \
		[ -d H4DAQ ] || git clone -b ${mybranch} git@github.com:cmsromadaq/H4DAQ.git ; \
		cd H4DAQ ;  \
                rm -rf Makefile;
		git pull ; \
		git log --oneline -n1 | sed \"s/^.*$/%%% & %%%/\" ;  \
		git diff origin/${mybranch} | sed \"s/^.*$/@@@ & @@@/\" ;  \
		env python configure.py; \
		make -j 4;  \
		./bin/resetCrate -t ${vmecontroller} -d 0 -l 0 ;  "

mycommandPlusDQM="cd ${daqhome};  \
                mkdir -p DAQ ;  \
                cd DAQ ; \
                [ -d H4DAQ ] || git clone -b ${mybranch} git@github.com:cmsromadaq/H4DAQ.git ; \
                cd H4DAQ ;  \
                rm -rf Makefile;                                                                                                                                                                                                                            git pull ; \
                git log --oneline -n1 | sed \"s/^.*$/%%% & %%%/\" ;  \
                git diff origin/${mybranch} | sed \"s/^.*$/@@@ & @@@/\" ;  \
                env python configure.py --noroot ; \
                make -j 4;  \
                ./bin/resetCrate -t ${vmecontroller} -d 0 -l 0; \
                cd ${daqhome};  \
                cd DAQ ; \
                [ -d H4DQM ] || git clone -b ${mybranch} git@github.com:cmsromadaq/H4DQM.git ; \
                cd H4DQM ;  \
                git pull ; \
                git log --oneline -n1 | sed \"s/^.*$/%%% & %%%/\" ;  \
                git diff origin/${mybranch} | sed \"s/^.*$/@@@ & @@@/\" ;  \  
                make -j 4;  "


IFS=','

function col1 { while read line ; do echo "$line" | sed 's:@@@\(.*\)@@@:\x1b[01;41m\1\x1b[00m:g' ; done }
function col2 { while read line ; do echo "$line" | sed 's:%%%\(.*\)%%%:\x1b[01;31m\1\x1b[00m:g' ; done }

for machine in $dr ; do
    # if more then one DR runs on the same machine they are separeted by commas. e.g. dr:0:stic:10
    IFS=':' read -r -a num_deamon <<< "$machine"
    machine="${num_deamon[0]}"
    unset num_deamon[0]
    for dea in "${num_deamon[@]}"
    do
	if [ "${machine}" == "localhost" ]; then  sshcommand="ssh ${daquser}@${machine} "; else  sshcommand=""; fi
	mydataro="cd ${daqhome}; cd DAQ/H4DAQ/ ; nice -n +${nice} ./bin/datareadout  -d -c data/${tag}/config_${machine}_DR_${dea}.xml -v ${verbosity} -l ${logdir}/log_h4daq_datareadout_${dea}_\$(date +%s)_${daquser}.log  > ${logdir}/log_h4daq_start_dr_${machine}_${dea}_\$(date +%s)_${daquser}.log" 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydataro" ; continue; }
#	[ "${start_dr}" == "0" ] && continue;
	## compile
	[ "${norecompile}" == "1" ] || if [ "${machine}" == "localhost" ]; then /bin/bash -i -c "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log | col1 | col2;  else ssh ${daquser}@${machine} /bin/bash -i -c \'"${mycommand}"\' 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log | col1 | col2 ; fi
	## launch the daemon
	echo "-----------------------------"
	echo "START DATAREADOUT on $machine"
	echo "-----------------------------"
	if  [ "${machine}" == "localhost" ]; then
	    /bin/bash -i -c "${mydatato}" 2>&1 | tee  /tmp/log_h4daq_start_dr_${machine}_${dea}_$(date +%s)_${USER}.log;
	else
	    ssh ${daquser}@${machine} /bin/bash -i -c \'"${mydataro}"\' 2>&1 | tee  /tmp/log_h4daq_start_dr_${machine}_${dea}_$(date +%s)_${USER}.log;
	fi
    done
done

for machine in $rc ; do 
	myrc="cd ${daqhome}; cd DAQ/H4DAQ/ ; nice -n +${nice} ./bin/runcontrol  -d -c data/${tag}/config_${machine}_RC.xml -v ${verbosity} -l ${logdir}/log_h4daq_runcontrol_\$(date +%s)_${daquser}.log >  ${logdir}/log_h4daq_start_rc_${machine}_\$(date +%s)_${daquser}.log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$myrc" ; continue; }
#	[ "${start_rc}" == "0" ] && continue;
	## compile
	[ "${norecompile}" == "1" ] || if [ "${machine}" == "localhost" ]; then /bin/bash -i -c "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log | col1 | col2;  else ssh ${daquser}@${machine} /bin/bash -i -c \'"${mycommand}"\' 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log | col1 | col2 ; fi
	## launch the daemon
	echo "-----------------------------"
	echo "START RUNCONTROL on $machine"
	echo "-----------------------------"
	if  [ "${machine}" == "localhost" ]; then
	    /bin/bash -i -c "${myrc}" 2>&1 | tee  /tmp/log_h4daq_start_rc_${machine}_$(date +%s)_${USER}.log;
	else
	    ssh ${daquser}@${machine} /bin/bash -i -c \'"${myrc}"\' 2>&1 | tee  /tmp/log_h4daq_start_rc_${machine}_$(date +%s)_${USER}.log;
	fi
done

for machine in $eb ; do 
    if [ "${machine}" == "localhost" ]; then  sshcommand="ssh ${daquser}@${machine} "; else  sshcommand=""; fi
	myeb="cd ${daqhome}; cd DAQ/H4DAQ ; nice -n +${nice} ./bin/eventbuilder  -d -c data/${tag}/config_${machine}_EB.xml -v ${verbosity} -l ${logdir}/log_h4daq_eventbuilder_\$(date +%s)_${daquser}.log >  ${logdir}/log_h4daq_start_eb_${machine}_\$(date +%s)_${daquser}.log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$myeb" ; continue; }
#	[ "${start_eb}" == "0" ] && continue;
	## compile
	[ "${ebrecompile}" == "0" ] || if [ "${machine}" == "localhost" ]; then /bin/bash -i -c "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log | col1 | col2;  else ssh ${daquser}@${machine} /bin/bash -i -c \'"${mycommand}"\' 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log | col1 | col2 ; fi
	## launch the daemon
	echo "-----------------------------"
	echo "START EVENTBUILDER on $machine"
	echo "-----------------------------"
	if  [ "${machine}" == "localhost" ]; then
	    /bin/bash -i -c "${myeb}" 2>&1 | tee  /tmp/log_h4daq_start_eb_${machine}_$(date +%s)_${USER}.log;
	else
	    ssh ${daquser}@${machine} /bin/bash -i -c \'"${myeb}"\' 2>&1 | tee  /tmp/log_h4daq_start_eb_${machine}_$(date +%s)_${USER}.log;
	fi
done

for machine in $drcv ; do 
    if [ "${machine}" == "localhost" ]; then  sshcommand="ssh ${daquser}@${machine} "; else  sshcommand=""; fi
	mydrcv="cd ${daqhome}; cd DAQ/H4DAQ ; nice -n +${nice} ./bin/datareceiver  -d -c data/${tag}/config_${machine}_DRCV.xml -v ${verbosity} -l ${logdir}/log_h4daq_datareceiver_\$(date +%s)_${daquser}.log >  ${logdir}/log_h4daq_start_drcv_${machine}_\$(date +%s)_${daquser}.log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommandPlusDQM" ; echo "$mydrcv" ; continue; }
#	[ "${start_drcv}" == "0" ] && continue;
	## compile
	[ "${drcvrecompile}" == "0" ] || if [ "${machine}" == "localhost" ]; then /bin/bash -i -c "${mycommandPlusDQM}" 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log | col1 | col2;  else ssh ${daquser}@${machine} "/bin/bash -c '${mycommandPlusDQM}' 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log " ; fi
	## launch the daemon
	echo "-----------------------------"
	echo "START DATARECEIVER on $machine"
	echo "-----------------------------"
	if  [ "${machine}" == "localhost" ]; then
	    /bin/bash -i -c "${mydrcv}" 2>&1 | tee  /tmp/log_h4daq_start_drcv_${machine}_$(date +%s)_${USER}.log;
	else
	    ssh ${daquser}@${machine} /bin/bash -i -c \'"${mydrcv}"\' 2>&1 | tee  /tmp/log_h4daq_start_drcv_${machine}_$(date +%s)_${USER}.log;
	fi
done



