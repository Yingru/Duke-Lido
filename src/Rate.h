#ifndef Rate_H
#define Rate_H

#include <cstdlib>
#include <vector>
#include <string>
#include <random>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "StochasticBase.h"
#include "Xsection.h"


// Cross-section based rate:
// N1: dimension of rate table, N2: dimension of Xsection table, 
// F: matrix element function type
template <size_t N1, size_t N2, typename F>
class Rate: public virtual StochasticBase<N1>{
private:
	std::shared_ptr<Xsection<N2, F>> X;
    scalar find_max(std::vector<double> parameters);
	scalar calculate_scalar(std::vector<double> parameters);
	fourvec calculate_fourvec(std::vector<double> parameters);
	tensor calculate_tensor(std::vector<double> parameters);
	double _mass, _degen;
	bool _active;
public:
	Rate(std::string Name, std::string configfile, F f);
	void sample(std::vector<double> arg, 
				std::vector< fourvec > & FS);
	void initX(std::string fname){X->init(fname);}
	void loadX(std::string fname){X->load(fname);}
	bool IsActive(void) {return _active;}
};

// Diffusion induced rate: (effective rate)
// N: dimension of rate table
// F: matrix element function type
// Given E, T, Delta t, determine rate and sample p' and k
// k could be initial or final state depending on which process it is
template <size_t N, typename F>
class EffRate: public virtual StochasticBase<N>{
private:
    scalar find_max(std::vector<double> parameters);
	scalar calculate_scalar(std::vector<double> parameters);
	fourvec calculate_fourvec(std::vector<double> parameters);
	tensor calculate_tensor(std::vector<double> parameters);
	F _f; // the kernel
	double _mass;
	bool _active;
public:
	EffRate(std::string Name, std::string configfile, F f);
	void sample(std::vector<double> arg, 
				std::vector< fourvec > & FS);
	bool IsActive(void) {return _active;}
};

#endif
