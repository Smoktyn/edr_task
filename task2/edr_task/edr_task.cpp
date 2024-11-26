#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include <json.hpp>
#include "edr_task.hpp"

int main()
{
	std::string filePath;
	std::cout << "Enter path to log file: ";
	std::getline(std::cin, filePath);

	std::ifstream file(filePath);

	if (!file.is_open()) {
		std::cerr << "Unable to open file" << std::endl;
		return 1;
	}
	
	CParser parser;
	parser.parse(file);

	file.close();
	return 0;
}
