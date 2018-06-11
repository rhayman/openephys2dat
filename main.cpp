#include <iostream>
#include <string>
#include <H5Cpp.h>
// #include <H5DataSet.h>

const H5std_string FILE_NAME( "/home/robin/Dropbox/Science/Recordings/OpenEphys/M3484/2018-06-05_14-21-37/experiment_1.nwb" );
const H5std_string DATASET_NAME("acquisition");

int main(int argc, char const *argv[])
{
	try {
		H5::Exception::dontPrint();
		H5::H5File file(FILE_NAME, H5F_ACC_RDONLY);
		H5::DataSet dataset = file.openDataSet(DATASET_NAME);
		H5T_class_t type_class = dataset.getTypeClass();
		std::cout << "type_class " << type_class << std::endl;
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