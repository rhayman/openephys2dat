#ifndef SCANHDF5_H_
#define SCANHDF5_H_

#include <hdf5.h>

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <stdio.h>
#include <wx/progdlg.h>

const unsigned int SAMPLE_RATE = 30000;

class wxProgressDialog;

class NwbData
{
private:
    H5::H5File * m_hdf_file;
    std::string m_filename;
public:
    NwbData(std::string filename) : m_filename(filename) {
        m_hdf_file = new H5::H5File(m_filename, H5F_ACC_RDONLY);
    };

    ~NwbData() {
        if ( m_hdf_file ) {
            m_hdf_file->close();
            delete m_hdf_file;
        }
    };

    std::map<std::string, std::string> getPaths(std::string file) {
        std::map<std::string, std::string> output;
        if ( m_hdf_file ) {
            hid_t fid = m_hdf_file->getId();
            H5Ovisit(fid, H5_INDEX_NAME, H5_ITER_NATIVE, op_func, (void*)&output);
        }
        return output;
    };

    std::vector<std::string> getMembers(hid_t fid) {
        std::vector<std::string> output;
        hsize_t idx;
        H5Literate(fid, H5_INDEX_NAME, H5_ITER_NATIVE, &idx, l_op_func, (void*)&output);
        return output;
    };

    void getDataSpaceDimensions(const std::string & pathToDataSet, int & nSamples, int & nChannels) {
        if ( m_hdf_file ) {
            if ( m_hdf_file->nameExists(pathToDataSet) ) {
                H5::DataSet dataset = m_hdf_file->openDataSet(pathToDataSet);
                H5::DataSpace dataspace = dataset.getSpace();
                hsize_t dims_out[2];
                int ndims = dataspace.getSimpleExtentDims(dims_out, NULL);
                nSamples = dims_out[0];
                nChannels = dims_out[1];
            }
        }
    };

    void ExportData(const std::string & pathToDataSet, const std::string & outputFname, const ExportParams & params, wxProgressDialog & prog) {
        if ( m_hdf_file ) {
            if ( m_hdf_file->nameExists(pathToDataSet) ) {
                H5::DataSet dataset = m_hdf_file->openDataSet(pathToDataSet);
                H5T_class_t type_class = dataset.getTypeClass();
                if ( type_class == H5T_INTEGER ) {
                    H5::IntType inttype = dataset.getIntType();
                    size_t precision = inttype.getPrecision();
                    H5::DataSpace dataspace = dataset.getSpace();
                    int rank = dataspace.getSimpleExtentNdims();
                    hsize_t dims_out[2];
                    int ndims = dataspace.getSimpleExtentDims(dims_out, NULL);
                    int nSamples = dims_out[0];
                    int nChannels = dims_out[1];
                    // Get the parameters specified in the controls
                    hsize_t start_sample = (hsize_t)params.m_start_time;
                    hsize_t end_sample = (hsize_t)params.m_end_time;
                    hsize_t start_channel = (hsize_t)params.m_start_channel;
                    hsize_t end_channel = (hsize_t)params.m_end_channel;
                    hsize_t sample_block_inc = 30000;
                    /*
                    The buffer the data is read into - if sample_block_inc = 3e4 and
                    nChannels = 64 then this buffer is about 3Mb big
                    */
                    int16_t data_out[sample_block_inc][end_channel-start_channel];
                    /*
                    dimsm:
                    an array that determines the dimensions used for the memory dataspace (see below)
                    */
                    hsize_t dimsm[2];
                    dimsm[0] = sample_block_inc;
                    dimsm[1] = end_channel-start_channel;
                    /*
                    offset:
                    determines the starting position in the dataspace
                    offset[0] = sample, offset[1] = channel
                    */
                    hsize_t offset[2];
                    offset[0] = start_sample;
                    offset[1] = start_channel;
                    /*
                    stride:
                    An array that allows you to sample elements along a dimension.
                    For example, a stride of one (or NULL) will select every element along a dimension,
                    a stride of two will select every other element,
                    and a stride of three will select an element after every two elements
                    */
                    hsize_t stride[2];
                    stride[0] = 1;
                    stride[1] = 1;
                    /*
                    block:
                    An array that determines the size of the element block selected from a dataspace.
                    If the block size is one or NULL then the block size is a single element in that dimension.
                    */
                    hsize_t block[2];
                    block[0] = 1;
                    block[1] = 1;
                    /*
                    count
                    An array that determines how many blocks to select from the dataspace in each dimension.
                    If the block size for a dimension is one then the count is the number of elements along that dimension.
                    */
                    hsize_t count[2]; // count
                    count[0] = sample_block_inc;
                    count[1] = end_channel-start_channel;
                    H5::DataSpace memspace(2, dimsm, NULL);
                    int inc = 0;
                    // Open the output file to write into
                    std::ofstream outfile(outputFname, std::ifstream::out);
                    for (int iSample = start_sample; iSample < end_sample; iSample+=sample_block_inc) {
                        offset[0] = iSample;
                        if ( (iSample + sample_block_inc) > end_sample ) {
                            hsize_t small_sample_block_inc = end_sample - iSample;
                            hsize_t small_count[2];
                            small_count[0] = small_sample_block_inc;
                            small_count[1] = end_channel-start_channel;
                            hsize_t small_dimsm[2];
                            small_dimsm[0] = small_sample_block_inc;
                            small_dimsm[1] = end_channel-start_channel;
                            H5::DataSpace small_memspace(2, small_dimsm, NULL);
                            int16_t small_data_chunk[small_sample_block_inc][end_channel-start_channel];
                            dataspace.selectHyperslab(H5S_SELECT_SET, small_count, offset, stride, block);
                            dataset.read(small_data_chunk, H5::PredType::NATIVE_INT16, small_memspace, dataspace);
                            outfile.write(reinterpret_cast<char*>(&small_data_chunk), sizeof(small_data_chunk));
                        }
                        else {
                            dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride, block);
                            dataset.read(data_out, H5::PredType::NATIVE_INT16, memspace, dataspace);
                            outfile.write(reinterpret_cast<char*>(&data_out), sizeof(data_out));
                        }
                        ++inc;
                        prog.Update(inc);
                    }
                    outfile.close();
                }
            }
        }
    };

    static herr_t op_func (hid_t loc_id, const char *name, const H5O_info_t *info, void *operator_data) {
        std::map<std::string, std::string> * v = static_cast<std::map<std::string, std::string>*>(operator_data);
        if (name[0] == '.') {}        /* Root group */
        else
            switch (info->type) {
                case H5O_TYPE_GROUP:
                    v->emplace(std::string(name), "Group");
                    break;
                case H5O_TYPE_DATASET:
                    v->emplace(std::string(name), "Dataset");
                    break;
                case H5O_TYPE_NAMED_DATATYPE:
                    v->emplace(std::string(name), "Datatype");
                    break;
                default:
                    v->emplace(std::string(name), "Unknown");
            }
        return 0;
    };

    static herr_t l_op_func (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data) {
        std::vector<std::string> * v = static_cast<std::vector<std::string>*>(operator_data);
        if (name[0] == '.')         /* Root group */
            {}
        else
            std::cout << "name " << name << ":\t" << info->type << std::endl;

        return 0;
    };
};

/************************************************************

  Operator function for H5Ovisit.  This function adds the type
  and path of all the objects in an HDF5 file to a map of
  <std::string, std::string>.

 ************************************************************/

herr_t op_func (hid_t loc_id, const char *name, const H5O_info_t *info,
            void *operator_data)
{
    std::map<std::string, std::string> * v = static_cast<std::map<std::string, std::string>*>(operator_data);
    if (name[0] == '.')         /* Root group */
        {}
    else        switch (info->type) {
            case H5O_TYPE_GROUP:
                v->emplace(std::string(name), "Group");
                break;
            case H5O_TYPE_DATASET:
                v->emplace(std::string(name), "Dataset");
                break;
            case H5O_TYPE_NAMED_DATATYPE:
                v->emplace(std::string(name), "Datatype");
                break;
            default:
                v->emplace(std::string(name), "Unknown");
        }
    return 0;
}
/************************************************************

  Operator function for H5Literate.  This function adds the type
  and path of all the objects in an HDF5 file to a vector of
  std::string.

 ************************************************************/
herr_t l_op_func (hid_t loc_id, const char *name, const H5L_info_t *info,
            void *operator_data)
{
    std::vector<std::string> * v = static_cast<std::vector<std::string>*>(operator_data);
    if (name[0] == '.')         /* Root group */
        {}
    else
        std::cout << "name " << name << ":\t" << info->type << std::endl;

    return 0;
}

std::map<std::string, std::string> getPaths(std::string file)
{
	hid_t fid = H5Fopen(file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

	std::map<std::string, std::string> output;
	H5Ovisit(fid, H5_INDEX_NAME, H5_ITER_NATIVE, op_func, (void*)&output);
	H5Fclose(fid);
	return output;
}

std::vector<std::string> getMembers(hid_t fid)
{
    std::vector<std::string> output;
    hsize_t idx;
    H5Literate(fid, H5_INDEX_NAME, H5_ITER_NATIVE, &idx, l_op_func, (void*)&output);
    return output;
}

#endif