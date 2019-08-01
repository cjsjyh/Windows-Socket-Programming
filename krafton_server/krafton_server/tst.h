#pragma once

class tst {
public:
	friend class boost::serialization::access;

	tst(std::string sName, int sage, float spi)
		:Name(sName), age(sage), pi(spi)
	{}

	tst() {
	}

	~tst() {

	}

	std::string Name;
	int age;
	float pi;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& Name;
		ar& age;
		ar& pi;
	}


};