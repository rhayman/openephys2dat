#include <iostream>
#include <string>
#include <H5Cpp.h>
// #include <H5DataSet.h>

const H5std_string FILE_NAME( "/home/robin/Dropbox/Science/Recordings/OpenEphys/M3484/2018-06-05_14-21-37/experiment_1.nwb" );
const H5std_string GROUP_NAME("acquisition");

int main(int argc, char const *argv[])
{
	try {
		H5::Exception::dontPrint();
		H5::H5File file(FILE_NAME, H5F_ACC_RDONLY);
		H5::Group group = file.openGroup(GROUP_NAME);
		hsize_t num_objs = group.getNumObjs();
		hid_t id = group.getId();
		std::cout << "num_objs " << num_objs << std::endl;
		std::cout << "id " << id << std::endl;
		H5std_string name = group.getObjnameByIdx(0);
		std::cout << "name " << name << std::endl;
		H5G_obj_t obj_type = group.getObjTypeByIdx(0);
		std::cout << "obj_type " << obj_type << std::endl;
		if ( obj_type == H5G_GROUP ) {
			std::cout << "obj_type is a group so opening\n";
			H5::Group new_group = group.openGroup(name);
			num_objs = new_group.getNumObjs();
			std::cout << "num_objs in new_group " << num_objs << std::endl;
			obj_type = new_group.getObjTypeByIdx(0);
			if ( obj_type == H5G_GROUP ) {
				name = new_group.getObjnameByIdx(0);
				std::cout << "new_group contains another group called " << name << std::endl;
			}
		}
		// hsize_t num_groups = group.getNumMembers(H5I_GROUP);
		// std::cout << "num_groups " << num_groups << std::endl;
	}
	catch ( H5::FileIException error ) {
		error.printError();
		return -1;
	}
	catch ( H5::DataSetIException error ) {
		error.printError();
		return -1;
	}
	return 0;
}