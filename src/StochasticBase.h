#ifndef StochasticBase_H
#define StochasticBase_H

#include <cstdlib>
#include <vector>
#include <string>
#include <random>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "TableBase.h"
//	double (*dXdPS)(double * PS, size_t n_dims, void * params);
// this base defines how a stochasitc object
// 1) calculate the probablity of a event with input parameters (0th moment)
// 2) calculate the first and second moments for the output parameters
// 3) tabulate with I/O of the probablity
// 4) interpolate the probablity over input parameters
// 5) given inputs, sample the output parameters

template <size_t N>
class StochasticBase{
protected:
	const std::string _Name;
	// 0-th moments of the distribution function
	// i.e. the integrated distribution function
    std::shared_ptr<TableBase<scalar, N>> _ZeroMoment;
	// 1-st moments of the distribution: <p^mu>
    std::shared_ptr<TableBase<fourvec, N>> _FirstMoment;
	// 2-nd moments of the distribution: <p^mu p^nu>, i.e. the correlator
    std::shared_ptr<TableBase<tensor, N>> _SecondMoment;
	virtual void compute(void) = 0;
public:
	StochasticBase(std::string Name, boost::property_tree::ptree config);
	scalar GetZeroM(std::vector<double> arg) {
			return _ZeroMoment->InterpolateTable(arg);}; 
	fourvec GetFirstM(std::vector<double> arg) {
			return _FirstMoment->InterpolateTable(arg);}; 
	tensor GetSecondM(std::vector<double> arg) {
			return _SecondMoment->InterpolateTable(arg);}; 
	virtual void sample(std::vector<double> arg, 
						std::vector< std::vector<double> > & FS) = 0;
	virtual void init(void) = 0;
};

#endif
