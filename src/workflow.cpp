#include "workflow.h"
#include "simpleLogger.h"
#include <fstream>
#include "random.h"
#include "matrix_elements.h"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/variant/get.hpp>
#include <boost/any.hpp>
#include <boost/foreach.hpp>
std::vector<Process> MyProcesses;

void initialize(std::string mode){
	
	boost::property_tree::ptree config;
    std::ifstream input("settings.xml");
    read_xml(input, config);
	double mu = config.get_child("Boltzmann.QCD").get<double>("mu");
    initialize_mD_and_scale(0, mu);

	MyProcesses.clear();
	MyProcesses.push_back( Rate22("Boltzmann/Qq2Qq", "settings.xml", dX_Qq2Qq_dt) );
	MyProcesses.push_back( Rate22("Boltzmann/Qg2Qg", "settings.xml", dX_Qg2Qg_dt) );
	
	MyProcesses.push_back( Rate23("Boltzmann/Qq2Qqg", "settings.xml", M2_Qq2Qqg) );
	MyProcesses.push_back( Rate23("Boltzmann/Qg2Qgg", "settings.xml", M2_Qg2Qgg) );

	BOOST_FOREACH(Process& r, MyProcesses){
		switch(r.which()){
			case 0:
				if (boost::get<Rate22>(r).IsActive())
					if(mode == "new"){
						boost::get<Rate22>(r).initX("table.h5");
						boost::get<Rate22>(r).init("table.h5");
					} else{
						boost::get<Rate22>(r).loadX("table.h5");
						boost::get<Rate22>(r).load("table.h5");
					}
				else continue;
				break;
			case 1:
				if (boost::get<Rate23>(r).IsActive())
					if(mode == "new"){
						boost::get<Rate23>(r).initX("table.h5");
						boost::get<Rate23>(r).init("table.h5");
					} else{
						boost::get<Rate23>(r).loadX("table.h5");
						boost::get<Rate23>(r).load("table.h5");
					}
				else continue;
				break;
			default:
				exit(-1);
				break;
		}
	}
}

int update_particle_momentum(double dt, double temp, std::vector<double> v3cell, 
				double D_formation_t, fourvec incoming_p, std::vector<fourvec> & FS){
	auto p_cell = incoming_p.boost_to(v3cell[0], v3cell[1], v3cell[2]);
	double D_formation_t_cell = D_formation_t / incoming_p.t() * p_cell.t();
	double dt_cell = dt / incoming_p.t() * p_cell.t();
	double E_cell = p_cell.t();
    std::vector<double> P_channels(MyProcesses.size());
	double P_total = 0.;
	int channel = 0;
	double dR;
	BOOST_FOREACH(Process& r, MyProcesses){
		switch(r.which()){
			case 0:
				if (boost::get<Rate22>(r).IsActive())
					dR = boost::get<Rate22>(r).GetZeroM(
												{E_cell, temp}).s * dt_cell;
				else dR = 0.0;
				P_channels[channel] = P_total + dR;	
				break;
			case 1:
				if (boost::get<Rate23>(r).IsActive())
					dR = boost::get<Rate23>(r).GetZeroM(
									{E_cell, temp, D_formation_t_cell}).s * dt_cell;
				else dR = 0.0;
				P_channels[channel] = P_total + dR;
				break;
			default:
				exit(-1);
				break;
		}
		P_total += dR;
		channel ++;
	}
	for(auto& item : P_channels) {item /= P_total;}
	if (P_total > 0.15) LOG_WARNING << "P_total = " << P_total << " may be too large";
	if ( Srandom::init_dis(Srandom::gen) > P_total) return -1;
	else{
		double p = Srandom::init_dis(Srandom::gen);
		for(int i=0; i<P_channels.size(); ++i){
			if (P_channels[i] > p) {
				channel = i; 
				break;
			}
		}
	}
	// Do scattering
	switch(MyProcesses[channel].which()){
		case 0:
			boost::get<Rate22>(MyProcesses[channel]).sample({E_cell, temp}, FS);
			break;
		case 1:
			boost::get<Rate23>(MyProcesses[channel]).sample(
											{E_cell, temp, D_formation_t_cell}, FS);
			break;
		default:
			LOG_FATAL << "Channel = " << channel << " not exists";
			exit(-1);
			break;
	}
	// rotate it back and boost it back
	for(auto & pmu : FS) {
		pmu = pmu.rotate_back(p_cell);
		pmu = pmu.boost_back(v3cell[0], v3cell[1], v3cell[2]);
	}
	return channel;
}

void probe_test(double E0, double T, double dt=0.05, int Nsteps=100, int Nparticles=10000, std::string mode="old"){
	double fmc_to_GeV_m1 = 5.026;
	initialize(mode);
	double M = 1.3;
	
	std::vector<particle> plist(Nparticles);
	for (auto & p : plist) { 
		p.pid = 4;
		p.x = fourvec{0,0,0,0};
		p.p = fourvec{E0, 0, 0, std::sqrt(E0*E0-M*M)};
		p.t_rad = 0.;
	}
	std::ofstream history("history.dat");
	double time = 0.;
	for (int it=0; it<Nsteps; ++it){
		time += dt;
		if (it%10 ==0) {
			LOG_INFO << it << " steps, " << "time = " << time << " [fm/c]";
			history << "#" << time << std::endl;
			for (auto & p : plist) history << p.x << " " << p.p << std::endl;
		}
		for (auto & p : plist){
			std::vector<fourvec> FS;
			int channel = update_particle_momentum(dt*fmc_to_GeV_m1, T, 
				{0.0, 0.0, 0.0}, (p.x.t()-p.t_rad)*fmc_to_GeV_m1, p.p, FS);
			
			p.freestream(dt);
			if (channel>=0) { 
				p.p = FS[0];
				if (channel == 2 || channel ==3) p.t_rad = time;
			}
		}
	}
}