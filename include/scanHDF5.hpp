#include <hdf5.h>

#include <vector>
#include <string>
#include <map>
#include <stdio.h>
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
    {
        std::cout << "name " << name << ":\t" << info->type << std::endl;
        
    }

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