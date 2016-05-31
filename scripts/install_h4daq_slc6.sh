#!/bin/bash

function post_install ()
{
    # DRYRUN
    echo ">> POST INSTALL"
    if [ ${1} == 1 ]; then
	return 
    fi
    # POST INSTALL
    /usr/sbin/lcm --configure ntpd afscl
    cat /etc/sysconfig/yum-autoupdate | sed -e 's:YUMUPDATE=1:YUMUPDATE=0:g' > /etc/sysconfig/yum-autoupdate.tmp
    mv /etc/sysconfig/yum-autoupdate.tmp /etc/sysconfig/yum-autoupdate
    /sbin/chkconfig --add yum-autoupdate
    /sbin/service yum-autoupdate start
    /usr/sbin/lcm --configure srvtab
    /usr/sbin/lcm --configure krb5clt sendmail ntpd chkconfig ocsagent ssh
    cern-config-users --setup-sudo-access --setup-user-accounts --setup-printers additional-sudoers
}

function root ()
{
    # DRYRUN
    echo ">> ROOT"
    if [ ${1} == 1 ]; then
	return 
    fi
    # install pkgs
    yum install -y sl-release-scl gsl-devel
    yum install -y freenx nxagent nxauth nxproxy xorg-x11-apps screen
    yum install -y git make binutils libX11-devel libXpm-devel libXft-devel libXext-devel
    yum install -y devtoolset-3-gcc-* devtoolset-3-gdb devtoolset-3-binutils devtoolset-3-elfutils python27* 
    source /opt/rh/python27/enable
    source /opt/rh/devtoolset-3/enable
    echo "source /opt/rh/devtoolset-3/enable" > /etc/profile.d/zzzz_dvts-3.sh
    echo "source /opt/rh/python27/enable" > /etc/profile.d/zzzz_py27.sh
    echo "source /opt/root/bin/thisroot.sh" > /etc/profile.d/root6.sh

    cd /tmp
    wget https://root.cern.ch/download/root_v6.04.14.source.tar.gz
    cd /opt
    tar -xzf /tmp/root_v6.04.14.source.tar.gz; mv root-6.04.14 root
    cd root/
    ./configure --prefix=/usr --enable-genvector --enable-mathmore --enable-minuit2 --enable-opengl --enable-python --enable-reflex --enable-roofit --enable-xrootd
    make -j 3
    make install
}

function zeromq ()
{
    # DRYRUN
    echo ">> ZEROMQ"
    if [ ${1} == 1 ]; then
	return 
    fi
    # get ZEROMQ
    yum install -y libsodium libsodium-devel
    cd /tmp/
    curl -o zeromq-4.1.4.tar.gz https://cernbox.cern.ch/index.php/s/SBUmAYCdRv2I2rK/download 
    cd /opt
    tar -xzf /tmp/zeromq-4.1.4.tar.gz 
    cd zeromq-4.1.4
    ./configure --prefix=/usr
    make -j 3
    make install
    cd /tmp
    wget https://raw.githubusercontent.com/zeromq/cppzmq/master/zmq.hpp
    cp zmq.hpp /usr/include/
    ldconfig
    easy_install-2.6 pyzmq
}

function caenlib ()
{
    # DRYRUN
    echo ">> CAENLIB"
    if [ ${1} == 1 ]; then
	return 
    fi
    # get CAEN lib
    cd /tmp
    curl -o CAENComm-1.2-build20140211.tgz https://cernbox.cern.ch/index.php/s/wHT5e5aJOPencyy/download
#    curl -o CAENDigitizer_2.6.8.tgz https://cernbox.cern.ch/index.php/s/Z5uzpOJehmQphZA/download
    curl -o CAENDigitizer_2.4.6.tgz https://cernbox.cern.ch/index.php/s/rQc8dYOyBjujZwH/download
    curl -o CAENUpgrader-1.5.2-build20120724.tgz https://cernbox.cern.ch/index.php/s/sONBXV0iLl8pf3r/download
    curl -o CAENVMELib-2.50.tgz https://cernbox.cern.ch/index.php/s/pTKdUwIZsMayRKr/download
    cd /opt
    tar -xzf /tmp/CAENComm-1.2-build20140211.tgz 
#    tar -xzf /tmp/CAENDigitizer_2.6.8.tgz 
    tar -xzf /tmp/CAENDigitizer_2.4.6.tgz 
    tar -xzf /tmp/CAENUpgrader-1.5.2-build20120724.tgz 
    tar -xzf /tmp/CAENVMELib-2.50.tgz

    cd CAENVMELib-2.50/lib/
    ./install_x64
    cd ../../CAENComm-1.2/lib/
    ./install_x64
#    cd ../../CAENDigitizer_2.6.8/
    cd ../../CAENDigitizer_2.4.6/
    ./install_64
    cd ../CAENUpgrader-1.5.2
    ./configure
    make -j 4
    make install
    ldconfig
}

function a2818_driver()
{
    # DRYRUN
    echo ">> A2818"
    if [ ${1} == 1 ]; then
	return 
    fi
    # get CAEN lib
    cd /tmp
    curl -o A2818Drv-1.19.tgz https://cernbox.cern.ch/index.php/s/sB4GVBXEWm36xwi/download
    cd /opt
    tar -xzf /tmp/A2818Drv-1.19.tgz
    cd A2818Drv-1.19
    make
    sh a2818_load
    cat <<EOF >> /etc/rc.local
. /etc/rc.d/init.d/functions
echo -n "Loading A2818 kernel driver"
cd /opt/A2818Drv-1.19
/opt/A2818Drv-1.19/a2818_load && success || failure
EOF
}

function arduino ()
{
    # DRYRUN
    echo ">> ARDUINO"
    if [ ${1} == 1 ]; then
	return 
    fi
    # get arduino lib
    cd /tmp
    curl -o arduino-1.6.8-linux64.tar.xz https://cernbox.cern.ch/index.php/s/6ZF6oZxvRyfKkur/download
    cd /opt
    tar -xf /tmp/arduino-1.6.8-linux64.tar.xz
    
}

function useradd ()
{
    # DRYRUN
    echo ">> USERADD"
    if [ ${1} == 1 ]; then
	return 
    fi
    # set cmsdaq user
    /usr/sbin/useradd -d /home/cmsdaq -U -G dialout -p '$1$LVNQmNAw$ySPQzDdy3cG5KogZL.5Qf/' -m cmsdaq
    su cmsdaq <<EOF
    mkdir -p /home/cmsdaq/.ssh
    echo 'ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAkrNSeBxzuZlyTp0ni28eyU2EogwedJCZuZ/e+6stkM1iwXAdPzcbm6BK1VAdzCOlqGzu8RueDit6KNvWzCzqLeJcIU5K46OgWcNZDuf25Qxe0rNbxrYP6JCpLojaPdXDSGTrCxza+7VJTUN680ew39GFzOBMy1gV5G9VPdeX37dwutCeQ0JN3hb3MLbHXILC0ZqjeU6HuuaT+Ev3/B86lTCICUy/bEL6UyhQrco75Oo7zeQkPt3dKNcFESiNJHgzuwvoGtkjud04uEYdldcZWcQ3XQTRC3bFblei/TFiyxmoAtphO7b5+QCoaAHuoTkxFqqcmZPbslkWazcviajAjw== cmsdaq@myhost' > /home/cmsdaq/.ssh/id_rsa.pub
    curl -o /home/cmsdaq/.ssh/id_rsa https://cernbox.cern.ch/index.php/s/PkRK1Dkb2ibZUHo/download
    chmod 0600 /home/cmsdaq/.ssh/id_rsa
    cat << EOL > /home/cmsdaq/.k5login
meridian@CERN.CH
spigazzi@CERN.CH
michelif@CERN.CH
shervin@CERN.CH
bmarzocc@CERN.CH
amartell@CERN.CH
EOL
EOF
}

function h4sw ()
{
    # DRYRUN
    echo ">> H4SW"
    if [ ${1} == 1 ]; then
	return 
    fi
    cat <<EOF > /etc/sysconfig/iptables
# Firewall configuration written by system-config-firewall                                                                                                                      
# Manual customization of this file is not recommended.                                                                                                                          
*filter
:INPUT ACCEPT [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
-A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
-A INPUT -p icmp -j ACCEPT
-A INPUT -i lo -j ACCEPT
-A INPUT -m state --state NEW -m tcp -p tcp --dport 22 -j ACCEPT
-A INPUT -m state --state NEW -m udp -p udp --dport 7001 -j ACCEPT
-A INPUT -m state --state NEW -m tcp -p tcp --dport 5000:6999 -j ACCEPT
-A INPUT -m state --state NEW -m tcp -p tcp --dport 80 -j ACCEPT
-A INPUT -j REJECT --reject-with icmp-host-prohibited
-A FORWARD -j REJECT --reject-with icmp-host-prohibited
COMMIT
EOF
    # install H4DAQ H4DQM H4GUI H4Analysis as cmsdaq
    yum -y install mysql-connector-python.noarch
    source /opt/root/bin/thisroot.sh
    source /opt/rh/python27/enable
    source /opt/rh/devtoolset-3/enable
    gcc --version
    su cmsdaq <<EOF
    mkdir -p /home/cmsdaq/DAQ
    cd /home/cmsdaq/DAQ
    ssh-keyscan github.com >> /home/cmsdaq/.ssh/known_hosts
    git clone -b master git@github.com:cmsromadaq/H4DAQ.git
    git clone -b master git@github.com:cmsromadaq/H4DQM.git
    git clone -b master git@github.com:cmsromadaq/H4GUI.git
    git clone --recursive git@github.com:simonepigazzini/H4Analysis.git
    cd /home/cmsdaq/DAQ/H4DAQ
    ./configure.py
    make -j 4
    cd /home/cmsdaq/DAQ/H4DQM
    make -j 4
    cd /home/cmsdaq/DAQ/H4Analysis
    make
    cd /home/cmsdaq/DAQ/H4GUI
    echo "db...cmsdaq" > /home/cmsdaq/DAQ/H4GUI/cmsdaq_mysql.passwd 
EOF
}

function mysql_db()
{
   # DRYRUN
    echo ">> MYSQL"
    if [ ${1} == 1 ]; then
	return 
    fi
    yum install -y mysql-server mysql
    /etc/init.d/mysqld start
    mysqladmin -u root password db...h4daq
    /sbin/chkconfig mysqld --levels 35 on
    curl -o '/tmp/rundb_v2.mysql' https://cernbox.cern.ch/index.php/s/qMkuIN7WEDMuEOD/download
    /usr/bin/mysql -u root -p'db...h4daq' < /tmp/rundb_v2.mysql
}

function web_server()
{
   # DRYRUN
    echo ">> HTTPD"
    if [ ${1} == 1 ]; then
	return 
    fi

    yum install -y httpd
    service httpd start
    /sbin/chkconfig httpd --levels 35 on
    yum install -y php php-mysql
    cat <<EOF > /etc/httpd/conf.d/dqmsite.conf 
<Directory "/data/public_DQM_plots">
    AllowOverride None
    Options +Indexes
    IndexOptions +FancyIndexing
    Order allow,deny
    Allow from all
</Directory>
EOF
}

function data_disk()
{
   # DRYRUN
    echo ">> DATA DISK"
    if [ ${1} == 1 ]; then
	return 
    fi
    mkdir /data
    chown -R cmsdaq.cmsdaq /data/
    su cmsdaq <<EOF
    mkdir -p /data
    mkdir -p /data/raw/EB/
    mkdir -p /data/raw/DataTree/
    mkdir -p /data/DQM/
EOF
}

function progress_bar() {
# Process data
    let _progress=(${1}*100/${2}*100)/100
    let _done=(${_progress}*4)/10
    let _left=40-$_done
# Build progressbar string lengths
    _fill=$(printf "%${_done}s")
    _empty=$(printf "%${_left}s")
# Progress : [########################################] 100%
printf "\r                                                                                     "
printf "\r\rProgress : [${_fill// /#}${_empty// /-}] ${_progress}%% => $opt"
}

function usage()
{
    echo "Usage: $0 (-d for dryrun) (--opt) (-h for help)" 1>&2
    echo "Possible options are: ${options[@]}." 1>&2
    echo "Specify no option to run all the installation steps" 1>&2
    echo "Installation log file: install_h4daq.log" 1>&2
    exit 1
}

### MAIN ###
TEMP=`getopt -o dh --long dryrun,help,useradd,arduino,caenlib,a2818_driver,zeromq,root,post_install,h4sw,mysql_db,web_server,data_disk -n 'install_h4daq.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Options are wrong..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

all=1
dryrun=0
help=0

while true; do
    case "$1" in
	-d | --dryrun ) dryrun=1; shift;;
	-h | --help ) help=1; shift;;
	--post_install ) post_install=1; all=0; shift;;
	--root ) root=1; all=0; shift;;
	--zeromq ) zeromq=1; all=0; shift;;
	--caenlib ) caenlib=1; all=0; shift;;
	--a2818_driver ) a2818_driver=1; all=0; shift;;
	--arduino ) arduino=1; all=0; shift;;
	--useradd ) useradd=1; all=0; shift;;
	--h4sw ) h4sw=1; all=0; shift;;
	--mysql_db ) mysql_db=1; all=0; shift;;
	--web_server ) web_server=1; all=0; shift;;
	--data_disk ) data_disk=1; all=0; shift;;
	-- ) shift; break ;;
	* ) break ;;
    esac
done

options=("post_install" "root" "zeromq" "caenlib" "a2818_driver" "arduino" "useradd" "h4sw" "mysql_db" "web_server" "data_disk")
len=${#options[@]}
echo "H4DAQ installer V1.0"
if [ "$help" == 1 ]; then
    usage
fi
touch install_h4daq.log
i=1

if  [ "$all" != 1 ]; then
    len=1
fi
for opt in "${options[@]}"; 
do    
    eval var=\$$opt
    if [ "$all" == 1 ] || [ "$var" == 1 ]; then
    	$opt $dryrun 2>&1 >> install_h4daq.log
	if [ $? != 0 ]; then
	    tail -n 10 install_h4daq.log
	    exit $?
	fi
	progress_bar $((i++)) $len 
        sleep 0.5
    fi
done
echo
exit 0
