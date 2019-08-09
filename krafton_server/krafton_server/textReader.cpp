#include "textReader.h"

textReader::textReader()
{

}

textReader::~textReader()
{

}

bool textReader::ReadFile(string fname)
{
	fstream in(fname);
	string line;
	int pos;
	if (!in.is_open())
	{
		std::cout << "Failed to open file" << endl;
		return false;
	}

	while (getline(in, line))
	{
		vector<string> fields;
		if (line == "")
			break;

		while ((pos = line.find(',')) >= 0)
		{
			fields.push_back(line.substr(0, pos));
			line = line.substr(pos + 1);
		}
		if (line.length() > 0)
			fields.push_back(line);

		if (fields.size() < 3)
		{
			cout << "[WARNING] INCORRECT DATASHEET FORMAT!" << endl;
			return false;
		}

		if (fields[2] == "int")
			paramInt.insert(make_pair(fields[0], stoi(fields[1])));
		else if (fields[2] == "float")
			paramFloat.insert(make_pair(fields[0], stof(fields[1])));
		else if (fields[2] == "bool")
		{
			if (fields[2] == "true")
				paramBool.insert(make_pair(fields[0], true));
			else
				paramBool.insert(make_pair(fields[0], false));
		}
		else
			return false;
	}
	return true;
}