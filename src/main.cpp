#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include "../include/scanHDF5.hpp"

constexpr unsigned int NCHANNELS2EXTRACT = 64;

inline bool does_file_exist (const std::string& name) {
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

int main(int argc, char const *argv[])
{
    if ( argc <= 1 ) {
        return 1;
    }
    std::fstream infile{ argv[1] };

    std::string line;
    std::vector<std::string> file_list;
    int count = 0;
    while(std::getline(infile, line)) {
        if( does_file_exist(line) && count > 0 ) {
            file_list.push_back(line);
            ++count;
        }
        else if ( count > 0 && ! does_file_exist(line)) {
            std::cout << "File : " + line + " does not exist" << std::endl;
            return 1;
        }
    }

    for ( const auto & this_file : file_list ) {
        NwbData data{this_file};
        auto paths = data.getPaths();
        std::string path2data;
        std::vector<std::string> potentialPaths2Data;
        for ( auto p : paths ) {
            if ( p.second == "Dataset") {
                if ( p.first.find("continuous") != std::string::npos && p.first.find("data") != std::string::npos)
                    potentialPaths2Data.push_back(p.first);
            }
        }
        // Check if there is more than one path to 'continuous' 'data' which might mean that there is 'recording0'
        // and 'recording1' and maybe more. Go with the one with the most amount of data in i.e. assume the user
        // erroneously pressed record and aborted...
        std::string path2data2export;
        int nchans = 0;
        int nsamps = 0;
        if ( potentialPaths2Data.size() == 0 ) {
            std::cout << "No path to the continuous data found in file:\n" + this_file << std::endl;
            return 1;
        }
        else if ( potentialPaths2Data.size() == 1 ) {
            path2data2export = potentialPaths2Data[0];
            data.getDataSpaceDimensions(path2data, nsamps, nchans);
        }
        else {
            for ( const auto & thisPath : potentialPaths2Data ) {
                int these_nsamps, these_nchans;
                data.getDataSpaceDimensions(path2data, these_nsamps, these_nchans);
                if ( these_nsamps > nsamps ) {
                    path2data2export = thisPath;
                    nsamps = these_nsamps;
                    nchans = these_nchans;
                }
            }
        }
        ExportParams params;
        params.m_start_channel = 0;
        params.m_end_channel = NCHANNELS2EXTRACT;
        params.m_start_time = 0;
        params.m_end_time = nsamps;

        auto output_fname = this_file.substr(0,this_file.find_first_of('.')) + ".dat";
        data.ExportData(path2data, output_fname, params);
    }
    return 0;
}
