H4DAQ
=======

DAQ code for VME boards. Developed originally for test beam in H4 experimental area at CERN SPS North Area. 
Depends on zeromq, root, CAENVMElibs (CAENVMElib,CAENComm,CAENDigitizer). 
Requires a CAEN VME host bridge (CAEN V1718 (USB) or CAEN V2718 (Optical link)). 
Can also be used for direct communication with CAEN Digitizers without a host bridge (e.g. CAEN V1742).
It is supposed to be used together with H4DQM (https://github.com/cmsromadaq/H4DQM) and H4GUI (https://github.com/cmsromadaq/H4GUI) 

======
Installation on SLC6

Full installation of a new machine including all dependencies (devtoolset-3, ROOT, zeromq...). 
Create a new user ('cmsdaq') and an empty mysql RunDB

As root user run:
`curl -o 'install_h4daq.sh' https://raw.githubusercontent.com/cmsromadaq/H4DAQ/master/scripts/install_h4daq_slc6.sh`
`chmod +x install_h4daq.sh`
`./install_h4daq.sh`

======
Configuration

Configuration file examples are available in the data directory. Create a new 'tag' (e.g. T9_2016_04) and put all the xml files for all the machines in the DAQ configuration (configuration files should be named as <tag>/config_<machine name>_<machine type>.xml 

Possible machine types are:
RC run control machine, master VME crate
DR data readout machine, secondary VME crate
EB event builder machine
DRCV data receiver machine, designed to receive data from the event builder e.g. online DQM, online unpacker, backup data

To launch/test the daemons as cmsdaq user do:

`cd /home/cmsdaq/DAQ/H4DAQ; scripts/startall.sh --tag <tag_name> --rc <rc_machine> --dr <dr machine1>,<dr_machine2> --eb <eb_machine> --drcv <drcv machine1>,<drcv machine2>`

