#include "Rate.h"
#include "integrator.h"
#include "sampler.h"
#include "minimizer.h"
#include "random.h"
#include "approx_functions.h"

template <>
Rate<2, 2, double(*)(const double, void *)>::
	Rate(std::string Name, std::string configfile, double(*f)(const double, void *)):
StochasticBase<2>(Name+"/rate", configfile),
X(std::make_shared<Xsection<2, double(*)(const double, void *)>>(Name, configfile, f) )
{
	// read configfile
	boost::property_tree::ptree config;
	std::ifstream input(configfile);
	read_xml(input, config);

	std::vector<std::string> strs;
	boost::split(strs, Name, boost::is_any_of("/"));
	std::string model_name = strs[0];
	std::string process_name = strs[1];
	auto tree = config.get_child(model_name+"."+process_name);
	_mass = tree.get<double>("mass");
	_degen = tree.get<double>("degeneracy");
	_active = (tree.get<std::string>("<xmlattr>.status")=="active")?true:false;

	// Set Approximate function for X and dX_max
	StochasticBase<2>::_ZeroMoment->SetApproximateFunction(approx_R22);
	StochasticBase<2>::_FunctionMax->SetApproximateFunction(approx_dR22_max);
}

template <>
Rate<3, 3, double(*)(const double*, void *)>::
	Rate(std::string Name, std::string configfile, double(*f)(const double*, void *)):
StochasticBase<3>(Name+"/rate", configfile),
X(std::make_shared<Xsection<3, double(*)(const double*, void *)>>(Name, configfile, f) )
{
	// read configfile
	boost::property_tree::ptree config;
	std::ifstream input(configfile);
	read_xml(input, config);

	std::vector<std::string> strs;
	boost::split(strs, Name, boost::is_any_of("/"));
	std::string model_name = strs[0];
	std::string process_name = strs[1];
	auto tree = config.get_child(model_name+"."+process_name);
	_mass = tree.get<double>("mass");
	_degen = tree.get<double>("degeneracy");
	_active = (tree.get<std::string>("<xmlattr>.status")=="active")?true:false;

	// Set Approximate function for X and dX_max
	StochasticBase<3>::_ZeroMoment->SetApproximateFunction(approx_R23);
	StochasticBase<3>::_FunctionMax->SetApproximateFunction(approx_dR23_max);
}

template <>
Rate<3, 5, double(*)(const double*, void *)>::
	Rate(std::string Name, std::string configfile, double(*f)(const double*, void *)):
StochasticBase<3>(Name+"/rate", configfile),
X(std::make_shared<Xsection<5, double(*)(const double*, void *)>>(Name, configfile, f) )
{
	// read configfile
	boost::property_tree::ptree config;
	std::ifstream input(configfile);
	read_xml(input, config);

	std::vector<std::string> strs;
	boost::split(strs, Name, boost::is_any_of("/"));
	std::string model_name = strs[0];
	std::string process_name = strs[1];
	auto tree = config.get_child(model_name+"."+process_name);
	_mass = tree.get<double>("mass");
	_degen = tree.get<double>("degeneracy");
	_active = (tree.get<std::string>("<xmlattr>.status")=="active")?true:false;

	// Set Approximate function for X and dX_max
	//StochasticBase<3>::_ZeroMoment->SetApproximateFunction(approx_R23);
	//StochasticBase<3>::_FunctionMax->SetApproximateFunction(approx_dR23_max);
}

/*****************************************************************/
/*************************Sample dR ******************************/
/*****************************************************************/
/*------------------Implementation for 2 -> 2--------------------*/
template <>
void Rate<2, 2, double(*)(const double, void *)>::
		sample(std::vector<double> parameters,
			std::vector< fourvec > & final_states){
	double E = parameters[0];
	double T = parameters[1];
	double v1 = std::sqrt(1. - std::pow(_mass/E,2));
	auto dR_dxdy = [E, T, v1, this](const double * x){
		double M = this->_mass;
		double E2 = T*(std::exp(x[0])-1.), costheta = x[1];
		if (costheta > 1. || costheta < -1.) return 0.;
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sqrts = std::sqrt(s);
		double Xtot = this->X->GetZeroM({sqrts,T}).s;
		double Jacobian = E2 + T;
    	return 1./E*E2*std::exp(-E2/T)*(s-M*M)*2*Xtot/16./M_PI/M_PI*Jacobian;
	};
	auto res = sample_nd(dR_dxdy, 2, {{0., 3.}, {-1., 1.}},
						StochasticBase<2>::GetFmax(parameters).s);
	double E2 = T*(std::exp(res[0])-1.),
		   costheta = res[1];
	double sintheta = std::sqrt(1. - costheta*costheta);
	double s = 2.*E2*E*(1. - v1*costheta) + _mass*_mass;
	double sqrts = std::sqrt(s);
	X->sample({sqrts, T}, final_states);

    // give incoming partilce a random phi angle
	double phi = Srandom::dist_phi(Srandom::gen);
    // com velocity
    double vcom[3] = { E2*sintheta/(E2+E)*cos(phi),
						E2*sintheta/(E2+E)*sin(phi),
						 (E2*costheta+v1*E)/(E2+E)	};
	/*
	FS now is in Z-oriented CoM frame
	1) FS.rotate_back
	2) FS.boost_back
	*/
	fourvec p1{E, 0, 0, v1*E};
	auto p1com = p1.boost_to(vcom[0], vcom[1], vcom[2]);
	for(auto & p: final_states){
		p = p.rotate_back(p1com);
		p = p.boost_back(vcom[0], vcom[1], vcom[2]);
	}
}
/*------------------Implementation for 2 -> 3--------------------*/
template <>
void Rate<3, 3, double(*)(const double*, void *)>::
		sample(std::vector<double> parameters,
			std::vector< fourvec > & final_states){
	double E = parameters[0];
	double T = parameters[1];
	double delta_t = parameters[2];
	double v1 = std::sqrt(1. - std::pow(_mass/E,2));
	auto dR_dxdy = [E, T, delta_t, v1, this](const double * x){
		double M = this->_mass;
		double E2 = T*(std::exp(x[0])-1.), costheta = x[1];
		if (costheta > 1. || costheta < -1.) return 0.;
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sqrts = std::sqrt(s);
		// transform dt to center of mass frame
		fourvec dxmu = {delta_t, 0., 0., delta_t*v1};
    	double sintheta = std::sqrt(1. - costheta*costheta);
    	double vcom[3] = { E2*sintheta/(E2+E), 0., (E2*costheta+v1*E)/(E2+E) };
    	double dt_com = (dxmu.boost_to(vcom[0], vcom[1], vcom[2])).t();
    	// interp cross-section
		double Xtot = this->X->GetZeroM({sqrts, T, dt_com}).s;
		double Jacobian = E2 + T;
    	return 1./E*E2*std::exp(-E2/T)*(s-M*M)*2*Xtot/16./M_PI/M_PI*Jacobian;
	};
	auto res = sample_nd(dR_dxdy, 2, {{0., 3.}, {-1., 1.}},
						StochasticBase<3>::GetFmax(parameters).s);
	double E2 = T*(std::exp(res[0])-1.), costheta = res[1];
	double sintheta = std::sqrt(1. - costheta*costheta);
	double phi = Srandom::dist_phi(Srandom::gen);
	double cosphi = std::cos(phi), sinphi = std::sin(phi);
	double vcom[3] = { E2*sintheta/(E2+E)*cosphi,
						E2*sintheta/(E2+E)*sinphi,
						(E2*costheta+v1*E)/(E2+E) };
	fourvec dxmu = {delta_t, 0., 0., delta_t*v1};
	double dt_com = (dxmu.boost_to(vcom[0], vcom[1], vcom[2])).t();
	double s = 2.*E2*E*(1. - v1*costheta) + _mass*_mass;
	double sqrts = std::sqrt(s);
	X->sample({sqrts, T, dt_com}, final_states);

	fourvec p1{E, 0, 0, v1*E};
	auto p1com = p1.boost_to(vcom[0], vcom[1], vcom[2]);
	for(auto & p: final_states){
		p = p.rotate_back(p1com);
		p = p.boost_back(vcom[0], vcom[1], vcom[2]);
	}
}
/*------------------Implementation for 3 -> 2--------------------*/
template <>
void Rate<3, 5, double(*)(const double*, void *)>::
		sample(std::vector<double> parameters,
			std::vector< fourvec > & final_states){
}

/*****************************************************************/
/*************************Find dR_max ****************************/
/*****************************************************************/
/*------------------Implementation for 2 -> 2--------------------*/
template <>
scalar Rate<2, 2, double(*)(const double, void*)>::
		find_max(std::vector<double> parameters){
	double E = parameters[0];
	double T = parameters[1];
	auto dR_dxdy = [E, T, this](const double * x){
		double M = this->_mass;
		double v1 = std::sqrt(1. - M*M/E/E);
		double E2 = T*(std::exp(x[0])-1.), costheta = x[1];
		if (E2 <= 0. || costheta >= 1. || costheta <= -1.) return 0.;
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sqrts = std::sqrt(s);
		double Xtot = this->X->GetZeroM({sqrts,T}).s;
		double Jacobian = E2 + T;
    	return -1./E*E2*std::exp(-E2/T)*(s-M*M)*2*Xtot/16./M_PI/M_PI*Jacobian;
	};
	// use f(E(x), y)*dE/dx, x = log(1+E/T), y = costheta
    // x start from 1, y start from 0
    // x step 0.3, cosphi step 0.3
    // save a slightly larger fmax
	auto val = -minimize_nd(dR_dxdy, 2, {1., 0.}, {0.2, 0.2}, 1000, 1e-8)*1.5;
    return scalar{val};
}
/*------------------Implementation for 2 -> 3--------------------*/
template <>
scalar Rate<3, 3, double(*)(const double*, void*)>::
		find_max(std::vector<double> parameters){
	double E = parameters[0];
	double T = parameters[1];
	double delta_t = parameters[2];
	auto dR_dxdy = [E, T, delta_t, this](const double * x){
		double M = this->_mass;
		double v1 = std::sqrt(1. - M*M/E/E);
		double E2 = T*(std::exp(x[0])-1.), costheta = x[1];
		if (E2 <= 0. || costheta >= 1. || costheta <= -1.) return 0.;
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sqrts = std::sqrt(s);
		// transform dt to center of mass frame
		fourvec dxmu = {delta_t, 0., 0., delta_t*v1};
    	double sintheta = std::sqrt(1. - costheta*costheta);
    	double vcom[3] = { E2*sintheta/(E2+E), 0., (E2*costheta+v1*E)/(E2+E) };
    	double dt_com = (dxmu.boost_to(vcom[0], vcom[1], vcom[2])).t();
    	// interp Xsection
		double Xtot = this->X->GetZeroM({sqrts, T, dt_com}).s;
		double Jacobian = E2 + T;
    	return -1./E*E2*std::exp(-E2/T)*(s-M*M)*2*Xtot/16./M_PI/M_PI*Jacobian;
	};
	// use f(E(x), y)*dE/dx, x = log(1+E/T), y = costheta
    // x start from 1, y start from 0
    // x step 0.3, cosphi step 0.3
    // save a slightly larger fmax
	auto val = -minimize_nd(dR_dxdy, 2, {1., 0.}, {0.2, 0.2}, 1000, 1e-8)*1.5;
    return scalar{val};
}
/*------------------Implementation for 3 -> 2--------------------*/
template <>
scalar Rate<3, 5, double(*)(const double*, void*)>::
		find_max(std::vector<double> parameters){
    return scalar{1.0};
}

/*****************************************************************/
/*************************Integrate dR ***************************/
/*****************************************************************/
/*------------------Implementation for 2 -> 2--------------------*/
template <>
scalar Rate<2, 2, double(*)(const double, void*)>::
		calculate_scalar(std::vector<double> parameters){
	double E = parameters[0];
	double T = parameters[1];
	auto code = [E, T, this](const double * x){
		double M = this->_mass;
		double v1 = std::sqrt(1. - M*M/E/E);
		double E2 = x[0], costheta = x[1];
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sqrts = std::sqrt(s);
		double Xtot = this->X->GetZeroM({sqrts,T}).s;
    	std::vector<double> res{1./E*E2*std::exp(-E2/T)*(s-M*M)*2*Xtot/16./M_PI/M_PI};
		return res;
	};
	double xmin[2] = {0., -1.};
	double xmax[2] = {10.*T,1.};
	double err;
	auto val = quad_nd(code, 2, 1, xmin, xmax, err);
	return scalar{_degen*val[0]};
}
/*------------------Implementation for 2 -> 3--------------------*/
template <>
scalar Rate<3, 3, double(*)(const double*, void*)>::
		calculate_scalar(std::vector<double> parameters){
	double E = parameters[0];
	double T = parameters[1];
	double delta_t = parameters[2];
	auto code = [E, T, delta_t, this](const double * x){
		double M = this->_mass;
		double v1 = std::sqrt(1. - M*M/E/E);
		double E2 = x[0], costheta = x[1];
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sqrts = std::sqrt(s);
		// transform dt to center of mass frame
		fourvec dxmu = {delta_t, 0., 0., delta_t*v1};
    	double sintheta = std::sqrt(1. - costheta*costheta);
    	double vcom[3] = { E2*sintheta/(E2+E), 0., (E2*costheta+v1*E)/(E2+E) };
    	double dt_com = (dxmu.boost_to(vcom[0], vcom[1], vcom[2])).t();
    	// interp Xsection
		double Xtot = this->X->GetZeroM({sqrts, T, dt_com}).s;
    	std::vector<double> res{1./E*E2*std::exp(-E2/T)*(s-M*M)*2*Xtot/16./M_PI/M_PI};
		return res;
	};
	double xmin[2] = {0., -1.};
	double xmax[2] = {10.*T,1.};
	double err;
	auto val = quad_nd(code, 2, 1, xmin, xmax, err);
	return scalar{_degen*val[0]};
}
/*------------------Implementation for 3 -> 2--------------------*/
template <>
scalar Rate<3, 5, double(*)(const double*, void*)>::
		calculate_scalar(std::vector<double> parameters){
	double E = parameters[0];
	double T = parameters[1];
	double delta_t = parameters[2];
	// x are: k, E2, cosk, cos2, phi2
	auto code = [E, T, delta_t, this](const double * x){
		double M = this->_mass;
		double M2 = M*M;
		double k = x[0], E2 = x[1], cosk = x[2], cos2 = x[3], phi2 = x[4];
		double sink = std::sqrt(1.-cosk*cosk), sin2 = std::sqrt(1.-cos2*cos2);
		double cosphi2 = std::cos(phi2), sinphi2 = std::sin(phi2);
		double v1 = std::sqrt(1. - M*M/E/E);
		fourvec p1mu{E, 0, 0, v1*E};
		fourvec p2mu{E2, E2*sin2*cosphi2, E2*sin2*sinphi2, E2*cos2};
		fourvec kmu{k, k*sink, 0., k*cosk};
		fourvec Ptot = p1mu+p2mu+kmu, P12 = p1mu+p2mu, P1k = p1mu+kmu;
		fourvec dxmu = {delta_t, 0., 0., delta_t*v1};
		double s = dot(Ptot, Ptot), s12 = dot(P12, P12), s1k = dot(P1k, P1k);
		double sqrts = std::sqrt(s), sqrts12 = std::sqrt(s12), sqrts1k = std::sqrt(s1k);
    	double v12[3] = { P12.x()/P12.t(), P12.y()/P12.t(), P12.z()/P12.t() };
    	double dt12 = (dxmu.boost_to(v12[0], v12[1], v12[2])).t();
		double xinel = (s12-M2)/(s-M2), yinel = (s1k/s-M2/s12)/(1.-s12/s)/(1.-M2/s12);
    	// interp Xsection
		double Xtot = this->X->GetZeroM({sqrts, T, xinel, yinel, dt12}).s;
		return std::exp(-(k+E2)/T)*k*E2*Xtot/E/8./std::pow(2.*M_PI, 5);
	};
	double xmin[5] = {0.,   0.,  -1., -1, 0.};
	double xmax[5] = {10*T, 10*T, 1., 1., 2.*M_PI};
	double error;
	double res = vegas(code, 5, xmin, xmax, error);
	return scalar{_degen*res};
}


/*****************************************************************/
/*******************Integrate Delta_p^mu*dR **********************/
/*****************************************************************/
/*------------------No need to do this generally-----------------*/
template <size_t N1, size_t N2, typename F>
fourvec Rate<N1, N2, F>::calculate_fourvec(std::vector<double> parameters){
	return fourvec::unity();
}
/*------------------Implementation for 2 -> 2--------------------*/
template <>
fourvec Rate<2, 2, double(*)(const double, void*)>::
		calculate_fourvec(std::vector<double> parameters){
	double E = parameters[0];
	double T = parameters[1];
	auto code = [E, T, this](const double * x){
		double M = this->_mass;
		double v1 = std::sqrt(1. - M*M/E/E);
		double E2 = x[0], costheta = x[1];
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sintheta = std::sqrt(1. - costheta*costheta);
		double sqrts = std::sqrt(s);
		double vcom[3] = {E2*sintheta/(E2+E), 0., (E2*costheta+v1*E)/(E2+E)};
		fourvec p1{E, 0, 0, v1*E};
		// A vector in p1z(com)-oriented com frame
		auto fmu0 = this->X->GetFirstM({sqrts,T});
		// rotate it back from p1z(com)-oriented com frame
		auto fmu1 = fmu0.rotate_back(p1.boost_to(vcom[0], vcom[1], vcom[2]));
		// boost back to the matter frame
		auto fmu2 = fmu1.boost_back(vcom[0], vcom[1], vcom[2]);
		double common = 1./E*E2*std::exp(-E2/T)*(s-M*M)*2./16./M_PI/M_PI;
		fmu2 = fmu2 * common;
		// Set tranverse to zero due to azimuthal symmetry;
		std::vector<double> res{fmu2.t(), fmu2.z()};
		return res;
	};
	double xmin[2] = {0., -1.};
	double xmax[2] = {5.*T, 1.};
	double err;
	auto val = quad_nd(code, 2, 2, xmin, xmax, err);
	return fourvec{_degen*val[0], 0.0, 0.0, _degen*val[1]};
}

/*****************************************************************/
/***********Integrate Delta_p^mu*Delta_p^mu*dR *******************/
/*****************************************************************/
/*------------------No need to do this generally-----------------*/
template <size_t N1, size_t N2, typename F>
tensor Rate<N1, N2, F>::calculate_tensor(std::vector<double> parameters){
	return tensor::unity();
}
/*------------------Implementation for 2 -> 2--------------------*/
template <>
tensor Rate<2, 2, double(*)(const double, void*)>::
		calculate_tensor(std::vector<double> parameters){
	double E = parameters[0];
	double T = parameters[1];
	auto code = [E, T, this](const double * x){
		double M = this->_mass;
		double v1 = std::sqrt(1. - M*M/E/E);
		double E2 = x[0], costheta = x[1], phi = x[2];
		double s = 2.*E2*E*(1. - v1*costheta) + M*M;
		double sintheta = std::sqrt(1. - costheta*costheta);
		double sqrts = std::sqrt(s);
		double vcom[3] = {E2*sintheta/(E2+E)*cos(phi), E2*sintheta/(E2+E)*sin(phi),
						 (E2*costheta+v1*E)/(E2+E)};
		fourvec p1{E, 0, 0, v1*E};
		// A vector in p1z(com)-oriented com frame
		auto fmunu0 = this->X->GetSecondM({sqrts,T});
		// rotate it back from p1z(com)-oriented com frame
		auto fmunu1 = fmunu0.rotate_back(p1.boost_to(vcom[0], vcom[1], vcom[2]));
		// boost back to the matter frame
		auto fmunu2 = fmunu1.boost_back(vcom[0], vcom[1], vcom[2]);
		double common = 1./E*E2*std::exp(-E2/T)*(s-M*M)*2./32./std::pow(M_PI, 3);
		fmunu2 = fmunu2 * common;
		// Set tranverse to zero due to azimuthal symmetry;
		std::vector<double> res{fmunu2.T[0][0], fmunu2.T[1][1],
								fmunu2.T[2][2], fmunu2.T[3][3]};
		return res;
	};
	double xmin[3] = {0., -1., -M_PI};
	double xmax[3] = {5.*T, 1., M_PI};
	double err;
	auto val = quad_nd(code, 3, 4, xmin, xmax, err);
	return tensor{_degen*val[0], 0., 0., 0.,
				  0., _degen*val[1], 0., 0.,
				  0., 0., _degen*val[2], 0.,
				  0., 0., 0., _degen*val[3]};
}


template class Rate<2,2,double(*)(const double, void*)>; // For 2->2
template class Rate<3,3,double(*)(const double*, void*)>; // For 2->3
template class Rate<3,5,double(*)(const double*, void*)>; // For 3->2
