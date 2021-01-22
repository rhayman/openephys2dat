#include <sstream>
#include <iostream>
#include <string>
#include "../include/scanHDF5.hpp"

int main(int argc, char const *argv[])
{
    if ( argc <= 1 ) {
        return 1;
    }
    std::stringstream fname{ argv[1] };
    NwbData data{fname.str()};
    auto paths = data.getPaths();
    // std::cout << " ------------PATHS-------------" << std::endl;
    // for ( auto p : paths ) {
    //     std::cout << p.first << " : " << p.second << std::endl;
    // }
    std::string p = "acquisition/timeseries/recording0/continuous/processor101_101/data";
    int nchans, nsamps;
    data.getDataSpaceDimensions(p, nsamps, nchans);
    std::cout << "nchans = " << nchans << std::endl;
    std::cout << "nsamps = " << nsamps << std::endl;
    return 0;
}
