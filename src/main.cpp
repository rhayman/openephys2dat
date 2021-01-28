#include <sys/stat.h>
#include <stdlib.h>
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

// Split a string given a delimiter and either return in a
// pre-constructed vector (#1) or returns a new one (#2)
inline void split(const std::string &s, char delim, std::vector<std::string> &elems)
{
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim))
		elems.push_back(item);
};

inline std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

int main(int argc, char const *argv[])
{
    if ( argc <= 1 ) {
        return 1;
    }
    std::fstream infile{ argv[1] };
    std::string line, output_dir;
    // the first line should be the output directory so read that
    std::getline(infile, output_dir);
    std::vector<std::string> file_list;
    while(std::getline(infile, line)) {
        if( does_file_exist(line) ) {
            file_list.push_back(line);
        }
        else {
            std::cout << "File : " + line + " does not exist" << std::endl;
            infile.close();
            return 1;
        }
    }
    infile.close();
    // create an output stream for saving the number of samples each file consists of
    std::fstream outInfoStream(output_dir + "continuous_data_info.txt", std::fstream::out);
    std::vector<std::string> output_files;
    
    show_console_cursor(false);
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
        int nchans = 0;
        int nsamps = 0;
        if ( potentialPaths2Data.size() == 0 ) {
            std::cout << "No path to the continuous data found in file:\n" + this_file << std::endl;
            return 1;
        }
        else if ( potentialPaths2Data.size() == 1 ) {
            path2data = potentialPaths2Data[0];
            data.getDataSpaceDimensions(path2data, nsamps, nchans);
        }
        else {
            for ( const auto & thisPath : potentialPaths2Data ) {
                int these_nsamps, these_nchans;
                data.getDataSpaceDimensions(thisPath, these_nsamps, these_nchans);
                if ( these_nsamps > nsamps ) {
                    path2data = thisPath;
                    nsamps = these_nsamps;
                    nchans = these_nchans;
                }
            }
        }
        // get the components of the path to the data...
        std::vector<std::string> path2PosComponents = split(path2data, '/');
        // ... the 3rd of these ('recording1', 'recording2' etc) can change; the
        // code above ensures we have the 'largest' recording so construct the path to the 
        // position data here
        std::string path2PosData;
        if ( ! path2PosComponents.empty() ) {
            for (int i = 0; i < 3; ++i)
                path2PosData += path2PosComponents[i] + "/";
            path2PosData += "events/binary1";
        }
        else {
            std::cout << "Could not find pos data. Exiting" << std::endl;
            return 1;
        }
        // Export the pos data
        int npossamps, nposchans;
        // std::cout << "path2PosData = " << path2PosData << std::endl;
        data.getDataSpaceDimensions(path2PosData+"/data", npossamps, nposchans);
        ExportParams posParams;
        posParams.m_start_channel = 0;
        posParams.m_end_channel = 2;
        posParams.m_start_time = 0;
        posParams.m_end_time = npossamps;
        posParams.m_block_size = npossamps;

        std::cout << "Extracting data from " << this_file << std::endl;
        std::cout << "Exporting pos data..." << std::endl;
        auto output_pos_fname = this_file.substr(0,this_file.find_first_of('.')) + "_pos.npy";
        data.ExportPosData(path2PosData, output_pos_fname, posParams);
        std::cout << "Pos exported" << std::endl;

        // output some data to the "continuous_data_info.txt" file
        outInfoStream << this_file << "\t" << path2data << "\t" << nsamps << std::endl;
        ExportParams params;
        params.m_start_channel = 0;
        params.m_end_channel = NCHANNELS2EXTRACT;
        params.m_start_time = 0;
        params.m_end_time = nsamps;

        auto output_fname = this_file.substr(0,this_file.find_first_of('.')) + ".dat";
        output_files.push_back(output_fname);
        
        data.ExportData(path2data, output_fname, params);
        
    }
    show_console_cursor(true);
    
    outInfoStream.close();
    
    // use cat to concatenate the files into one final thing
    std::string catCommand = "cat ";
    for ( const auto & fname : output_files ) {
        catCommand += fname + " ";
    }
    catCommand += "> " + output_dir + "combined.dat";
    const char * command = catCommand.c_str();
    std::cout << "Concatenating data files..." << std::endl;
    std::system(command);
    // Delete the .dat files that make up the combined one
    for ( const auto & fname : output_files ) {
        catCommand = "rm " + fname;
        const char * command = catCommand.c_str();
        std::system(command);
    }
    
    return 0;
}
