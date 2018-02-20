#ifndef Xsection_H
#define Xsection_H

#include <cstdlib>
#include <vector>
#include <string>
#include <random>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "StochasticBase.h"

template <size_t N, typename F>
class Xsection: public StochasticBase<N> {
private:
	scalar calculate_scalar(std::vector<double> parameters);
	fourvec calculate_fourvec(std::vector<double> parameters);
	tensor calculate_tensor(std::vector<double> parameters);
	double _mass;
	F _f;// the matrix element
public:
	Xsection(std::string Name, boost::property_tree::ptree config, F f);
	void sample(std::vector<double> arg, 
						std::vector< std::vector<double> > & FS);
};

#endif