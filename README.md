## Synopsis

A standalone GUI that allows export of binary data from an open-ephys .nwb (hdf5) file

## Motivation

The dataset (in hdf5 parlance) that contains the neural data from an open-ephys recording
(when saved in *.nwb format) is buried along a long-ish path. You can navigate to this
location using h5py (python) and then export but that takes some time and the path may differ
between recordings. This GUI is agnostic to the path and just scans the whole hdf5 file and
shows only the 'datasets' of the .nwb file.

## Installation

Prerequisites include the following:

* a c++14 capable compiler
* cmake v3.0
* wxwidgets (tested with 3.1.0) - needs the gtk2 toolkit
* the hdf5 libraries

The main pain is the hdf5 libraries. The best way for me has been to download them from the hdfgroup
website:

> https://www.hdfgroup.org/downloads/hdf5/source-code/

Get hold of the bzip or whatever and extract it, go into the resulting folder and do:

```
./configure --prefix=/usr/local/hdf5 --enable-cxx
make
sudo make install
make check - optional
```

Then in the CMakeLists.txt file the line that reads:

> set( HDF5_ROOT  /usr/local/hdf5 )

Should mean that all the libraries, binaries etc should be the ones you need.

Download or clone the repo or whatever and then at the top level do:

```
mkdir bin
mkdir build
cd build
cmake ..
```

CMake wil check everything is ok - check messages for errors etc and fix any problems.
If no problems then do:

```
make
```

## Usage

Just run the resulting binary file in the 'bin' directory and a window will appear.

Open the .nwb file using Ctrl-O or use the File menu. Once open a list of available
datasets should appear in the main window. If you select one of the datasets the Time
and Channel boxes should update their values to show you what is available to export.

For example, my typical datasets consist of 70 channels recorded over ~20 minutes so have a
start time of 0.0000 and an end time of 1224.5600, a start channel of 0 and an end channel of 70.
For me the last 6 channels are data from the accelerometers on my headstage so when exporting
the data to a .dat file I select only the first 64 channels but the full time range. So select
whatever ranges you want and then click Export and save the file. The .dat file is now ready to
fire into Kilosort or Klusta or any other clustering/ spike assignment program.