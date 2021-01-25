#include <sstream>
#include <iostream>
#include <string>
#include "../include/scanHDF5.hpp"

constexpr unsigned int NCHANNELS2EXTRACT = 64;

int main(int argc, char const *argv[])
{
    if ( argc <= 1 ) {
        return 1;
    }
    std::stringstream fname{ argv[1] };
    NwbData data{fname.str()};
    auto paths = data.getPaths();
    std::string path2data;
    std::cout << " ------------PATHS-------------" << std::endl;
    for ( auto p : paths ) {
        if ( p.second == "Dataset") {
            if ( p.first.find("continuous") != std::string::npos && p.first.find("data") != std::string::npos)
                path2data = p.first;
        }
    }
     std::cout << "Path to continuous data : " << path2data << std::endl;
    // std::string p = "acquisition/timeseries/recording0/continuous/processor101_101/data";
    if ( ! path2data.empty() ) {
        int nchans, nsamps;
        data.getDataSpaceDimensions(path2data, nsamps, nchans);
        std::cout << "nchans = " << nchans << std::endl;
        std::cout << "nsamps = " << nsamps << std::endl;
    
        ExportParams params;
        params.m_start_channel = 0;
        params.m_end_channel = NCHANNELS2EXTRACT;
        params.m_start_time = 0;
        params.m_end_time = nsamps;

        std::string _output_fname = fname.str();
        auto output_fname = _output_fname.substr(0,_output_fname.find_first_of('.')) + ".dat";

        data.ExportData(path2data, output_fname, params);
    }
    return 0;
}
