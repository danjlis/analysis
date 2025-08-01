#include <iostream>
#include <TH2D.h>
#include <TH1D.h>
#include <TChain.h>
#include <TMath.h>
#include <TTree.h>
#include <TFile.h>
#include <sstream> //std::ostringstsream
#include <fstream> //std::ifstream
#include <iostream> //std::cout, std::endl
#include <cmath>
#include <TGraphErrors.h>
#include <TGraph.h>
#include <TSpectrum.h>
#include <TF1.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>
#include <string>
#include <set>
#include <TVector3.h>
#include <map>
#include <vector>
#include <TDatabasePDG.h>
#include <tuple>
#include <TProfile.h>
#include <TProfile2D.h>
#include "TH1D.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//																																			//
//	Note: 11.26.24 																															//
// 	This macro creates dijet QA plots, energy density QA plots with options for using towers, topoclusters or emcal clusters,				//
//	and plots of mean energy density vs jet variables (leading jet pT and dijet xj)															//
//																																			//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<int> run_numbers;
std::vector<double> collision_rates;
std::vector<double> pileup_rates;
//std::vector<double> pileup_bins = {0.0,0.015};
std::vector<double> pileup_bins = {0.0,0.02,0.03,0.04,0.05,0.06,0.07,0.1};
//std::vector<double> pileup_bins = {0.06,0.07,0.1};

void dijet_0mrad_ue_pileup_analysis(bool sim = true, bool clusters = true, bool emcal_clusters = false, bool applyCorr = false) {

    std::ifstream infile("0mrad_collision_rates.txt");
    if (!infile) {
        std::cerr << "Error: Unable to open input file!" << std::endl;
        return;
    }
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int run_number;
        double collision_rate;  
        if (!(iss >> run_number >> collision_rate)) {
            std::cerr << "Error: Malformed line in input file!" << std::endl;
            continue;
        }
        run_numbers.push_back(run_number);
        collision_rates.push_back(collision_rate);

        // Calculate lambda
        double denominator = 78000.0 * 111.0;
        double lambda = collision_rate / denominator;
        double exp_neg_lambda = exp(-lambda);
        
        // Calculate pk
        double pk = 1 - lambda * exp_neg_lambda - exp_neg_lambda;
        
        // Calculate pileup rate
        double pileup_rate = pk / (pk + lambda * exp_neg_lambda);
        pileup_rates.push_back(pileup_rate);
    }
    infile.close();

    // define constants 
	float deltaeta = 0.0916667;
	float deltaphi = 0.0981748;
	float secteta = 2.2;
	float sectphi = (2.0*M_PI)/3.0;
	float ptbins[] = {5,6,7,8,9,10,12,14,16,18,20,22,24,27,30,35,40,50,60,80};
    int nptbins = sizeof(ptbins) / sizeof(ptbins[0]) - 1;
    float etmin = -20;
    float etmax = 80;
    int netbins = int((10*(etmax - etmin)));
    float etbins[netbins+1];
    for (int i = 0; i <= netbins; i++) {
    	etbins[i] = etmin + i*(etmax - etmin) / netbins;
    }
    float jetetamax = 0.7;
    float lead_ptmin = 12;
    float sub_ptmin = 7;
    float deltaphimin = 3.0*M_PI/4.0;

    for (int pb = 1; pb < pileup_bins.size(); pb++) {
    	string outfilename = "pileup_0mrad_UE_analysis";
    	if (!clusters) outfilename += "_calo_tower_sum";
    	if (clusters && emcal_clusters) outfilename += "_emcal_clusters";
    	outfilename += "_leadjet_15_20_GeV_rate_range_" + to_string(pileup_bins[pb-1]) + "_" + to_string(pileup_bins[pb]) + ".root";
    	//outfilename += "_" + to_string(pileup_bins[pb]) +".root";
    	std::cout << "Creating " << outfilename << std::endl;
		TFile *out = new TFile(outfilename.c_str(),"RECREATE");
 
		// create event and jet histograms 
		TH1F* h_vz = new TH1F("h_vz","",400, -100, 100);
	  	TH1F* h_leadjet = new TH1F("h_leadjet","",nptbins, ptbins);
	  	TH1F* h_subjet = new TH1F("h_subjet","",nptbins, ptbins);
	  	TH1F* h_leadphi = new TH1F("h_leadphi","",50,-M_PI,M_PI);
	  	TH1F* h_leadeta = new TH1F("h_leadeta","",50,-1.1,1.1);
	  	TH1F* h_subphi = new TH1F("h_subphi","",50,-M_PI,M_PI);
	  	TH1F* h_subeta = new TH1F("h_subeta","",50,-1.1,1.1);
	  	TH1F* h_deltaphi = new TH1F("h_deltaphi","",125,-2*M_PI,2*M_PI);
	  	TH1F* h_pass_deltaphi = new TH1F("h_pass_deltaphi","",125,-2*M_PI,2*M_PI);
	  	TH1F* h_pass_xj = new TH1F("h_pass_xj","",20,0,1);
	  	TH1F* h_pass_spectra = new TH1F("h_pass_spectra","",nptbins, ptbins);
	  	TH2F* h_aj_ptavg = new TH2F("h_aj_ptavg","",100,0,100,100,0,1);

	  	// create UE histograms 
	  	TH2F* h_ue_2D_total = new TH2F("h_ue_2D_total","",24,-1.1,1.1,32,0,M_PI);
	  	TH2F* h_ue_2D_towards = new TH2F("h_ue_2D_towards","",24,-1.1,1.1,32,0,M_PI);
	  	TH2F* h_ue_2D_transverse = new TH2F("h_ue_2D_transverse","",24,-1.1,1.1,32,0,M_PI);
	  	TH2F* h_ue_2D_away = new TH2F("h_ue_2D_away","",24,-1.1,1.1,32,0,M_PI);

	  	TH2F* h_trans_et_2D_index_total = new TH2F("h_trans_et_2D_index_total","",24,0,24,64,0,64);
	  	TH2F* h_trans_et_2D_index_emcal = new TH2F("h_trans_et_2D_index_emcal","",96,0,96,256,0,256);
	  	TH2F* h_trans_et_2D_index_ihcal = new TH2F("h_trans_et_2D_index_ihcal","",24,0,24,64,0,64);
	  	TH2F* h_trans_et_2D_index_ohcal = new TH2F("h_trans_et_2D_index_ohcal","",24,0,24,64,0,64);

	  	TH1F* h_ue_towards = new TH1F("h_ue_towards","",netbins, etbins);
	  	TH1F* h_ue_transverse = new TH1F("h_ue_transverse","",netbins, etbins);
	  	TH1F* h_ue_away = new TH1F("h_ue_away","",netbins, etbins);

	  	TH1F* h_et_towards = new TH1F("h_et_towards","",netbins, etbins);
	  	TH1F* h_et_transverse = new TH1F("h_et_transverse","",netbins, etbins);
	  	TH1F* h_et_away = new TH1F("h_et_away","",netbins, etbins);

	  	// create topocluster histograms 
	  	int topo_thresholds[] = {-9999,0,100,200,300,500,1000,2000};

	  	TH1F* h_ntopo_towards[8];
	  	TH1F* h_ntopo_transverse[8];
	  	TH1F* h_ntopo_away[8];

	  	TH1F* h_topo_towards[8];
	  	TH1F* h_topo_transverse[8];
	  	TH1F* h_topo_away[8];

	  	TH2F* h_2D_topo_towards[8];
	  	TH2F* h_2D_topo_transverse[8];
	  	TH2F* h_2D_topo_away[8];

	  	TH1F* h_sume_topo_towards[8];
	  	TH1F* h_sume_topo_transverse[8];
	  	TH1F* h_sume_topo_away[8];

	  	for (int i = 0; i < 8; i++) {
	  		h_ntopo_towards[i] = new TH1F(Form("h_ntopo%d_towards",topo_thresholds[i]),"",200,0,200);
	  		h_ntopo_transverse[i] = new TH1F(Form("h_ntopo%d_transverse",topo_thresholds[i]),"",200,0,200);
	  		h_ntopo_away[i] = new TH1F(Form("h_ntopo%d_away",topo_thresholds[i]),"",200,0,200);

	  		h_topo_towards[i] = new TH1F(Form("h_topo%d_towards",topo_thresholds[i]),"",netbins, etbins);
	  		h_topo_transverse[i] = new TH1F(Form("h_topo%d_transverse",topo_thresholds[i]),"",netbins, etbins);
	  		h_topo_away[i] = new TH1F(Form("h_topo%d_away",topo_thresholds[i]),"",netbins, etbins);

	  		h_2D_topo_towards[i] = new TH2F(Form("h_2D_topo%d_towards",topo_thresholds[i]),"",24,-1.1,1.1,32,0,M_PI);
	  		h_2D_topo_transverse[i] = new TH2F(Form("h_2D_topo%d_transverse",topo_thresholds[i]),"",24,-1.1,1.1,32,0,M_PI);
	  		h_2D_topo_away[i] = new TH2F(Form("h_2D_topo%d_away",topo_thresholds[i]),"",24,-1.1,1.1,32,0,M_PI);

	  		h_sume_topo_towards[i] = new TH1F(Form("h_sume_topo%d_towards",topo_thresholds[i]),"",netbins, etbins);
	  		h_sume_topo_transverse[i] = new TH1F(Form("h_sume_topo%d_transverse",topo_thresholds[i]),"",netbins, etbins);
	  		h_sume_topo_away[i] = new TH1F(Form("h_sume_topo%d_away",topo_thresholds[i]),"",netbins, etbins);
	  	}

	  	// create histograms for UE versus jet variables 
	  	TProfile* h_ue_xj_towards = new TProfile("h_ue_xj_towards","",20,0,1);
	  	TProfile* h_ue_xj_transverse = new TProfile("h_ue_xj_transverse","",20,0,1);
	  	TProfile* h_ue_xj_away = new TProfile("h_ue_xj_away","",20,0,1);

	  	TProfile* h_ue_pt_towards = new TProfile("h_ue_pt_towards","",nptbins, ptbins);
	  	TProfile* h_ue_pt_transverse = new TProfile("h_ue_pt_transverse","",nptbins, ptbins);
	  	TProfile* h_ue_pt_away = new TProfile("h_ue_pt_away","",nptbins, ptbins);

	  	TH2D* h_et_pt_towards_2D = new TH2D("h_et_pt_towards_2D","",nptbins, ptbins, netbins, etbins);
	  	TH2D* h_et_pt_transverse_2D = new TH2D("h_et_pt_transverse_2D","", nptbins, ptbins, netbins, etbins);
	  	TH2D* h_et_pt_away_2D = new TH2D("h_et_pt_away_2D","", nptbins, ptbins, netbins, etbins);

	  	// input datasets from ttrees
	 	TChain chain("T");
	 	for (int i = 0; i < run_numbers.size(); i++) {
	 		if (pileup_rates[i] < pileup_bins[pb] && pileup_rates[i] > pileup_bins[pb-1]) {
	 			std::string wildcardPath = "/sphenix/tg/tg01/jets/egm2153/UEinppOutput/output_0mrad_ana468_" + std::to_string(run_numbers[i]) + "_*.root";
				chain.Add(wildcardPath.c_str());
	 		}
	 	}

		chain.SetBranchStatus("*",0);

		// define ttree branch variables 
		int m_event;
		int nJet;
		float zvtx;

		vector<int> *triggerVector = nullptr;
		vector<float> *eta = nullptr;
		vector<float> *phi = nullptr;
		vector<float> *e = nullptr;
		vector<float> *pt = nullptr;
		vector<float> *jetemcale = nullptr;
		vector<float> *jetihcale = nullptr;
		vector<float> *jetohcale = nullptr;

		int emcaln = 0;
		float emcale[24576] = {0.0};
		float emcaleta[24576] = {0.0};
		float emcalphi[24576] = {0.0};
		int emcalieta[24576] = {0};
		int emcaliphi[24576] = {0};

		int ihcaln = 0;
		float ihcale[24576] = {0.0};
		float ihcaleta[24576] = {0.0};
		float ihcalphi[24576] = {0.0};
		int ihcalieta[24576] = {0};
		int ihcaliphi[24576] = {0};

		int ohcaln = 0;
		float ohcale[24576] = {0.0};
		float ohcaleta[24576] = {0.0};
		float ohcalphi[24576] = {0.0};
		int ohcalieta[24576] = {0};
		int ohcaliphi[24576] = {0};

		int clsmult = 0;
		float cluster_e[10000] = {0.0};
		float cluster_eta[10000] = {0.0};
		float cluster_phi[10000] = {0.0};

		int truthpar_n = 0;
		float truthpar_e[100000] = {0};
		float truthpar_eta[100000] = {0};
		float truthpar_phi[100000] = {0};
		int truthpar_pid[100000] = {0};

		chain.SetBranchStatus("m_event",1);
		chain.SetBranchStatus("nJet",1);
		chain.SetBranchStatus("zvtx",1);
		chain.SetBranchStatus("triggerVector",1);
		chain.SetBranchStatus("eta",1);
		chain.SetBranchStatus("phi",1);
		chain.SetBranchStatus("e",1);
		chain.SetBranchStatus("pt",1);
		chain.SetBranchStatus("jetEmcalE", 1);
		chain.SetBranchStatus("jetIhcalE", 1);
		chain.SetBranchStatus("jetOhcalE", 1);

		chain.SetBranchAddress("m_event",&m_event);
		chain.SetBranchAddress("nJet",&nJet);
		chain.SetBranchAddress("zvtx",&zvtx);
		chain.SetBranchAddress("triggerVector",&triggerVector);
		chain.SetBranchAddress("eta",&eta);
		chain.SetBranchAddress("phi",&phi);
		chain.SetBranchAddress("e",&e);
		chain.SetBranchAddress("pt",&pt);
		chain.SetBranchAddress("jetEmcalE", &jetemcale);
		chain.SetBranchAddress("jetIhcalE", &jetihcale);
		chain.SetBranchAddress("jetOhcalE", &jetohcale);

		if (!clusters) {
			chain.SetBranchStatus("emcaln", 1);
			chain.SetBranchStatus("emcale",1);
			chain.SetBranchStatus("emcaleta", 1);
			chain.SetBranchStatus("emcalphi", 1);
			chain.SetBranchStatus("emcalieta", 1);
			chain.SetBranchStatus("emcaliphi", 1);

			chain.SetBranchStatus("ihcaln", 1);
			chain.SetBranchStatus("ihcale", 1);
			chain.SetBranchStatus("ihcaleta", 1);
			chain.SetBranchStatus("ihcalphi", 1);
			chain.SetBranchStatus("ihcalieta", 1);
			chain.SetBranchStatus("ihcaliphi", 1);

			chain.SetBranchStatus("ohcaln", 1);
			chain.SetBranchStatus("ohcale", 1);
			chain.SetBranchStatus("ohcaleta", 1);
			chain.SetBranchStatus("ohcalphi", 1);
			chain.SetBranchStatus("ohcalieta", 1);
			chain.SetBranchStatus("ohcaliphi", 1);

			chain.SetBranchAddress("emcaln",&emcaln);
			chain.SetBranchAddress("emcale",emcale);
			chain.SetBranchAddress("emcaleta",emcaleta);
			chain.SetBranchAddress("emcalphi",emcalphi);
			chain.SetBranchAddress("emcalieta",emcalieta);
			chain.SetBranchAddress("emcaliphi",emcaliphi);

			chain.SetBranchAddress("ihcaln",&ihcaln);
			chain.SetBranchAddress("ihcale",ihcale);
			chain.SetBranchAddress("ihcaleta",ihcaleta);
			chain.SetBranchAddress("ihcalphi",ihcalphi);
			chain.SetBranchAddress("ihcalieta",ihcalieta);
			chain.SetBranchAddress("ihcaliphi",ihcaliphi);

			chain.SetBranchAddress("ohcaln",&ohcaln);
			chain.SetBranchAddress("ohcale",ohcale);
			chain.SetBranchAddress("ohcaleta",ohcaleta);
			chain.SetBranchAddress("ohcalphi",ohcalphi);
			chain.SetBranchAddress("ohcalieta",ohcalieta);
			chain.SetBranchAddress("ohcaliphi",ohcaliphi);
		}

		if (!emcal_clusters) {
			chain.SetBranchStatus("clsmult", 1);
			chain.SetBranchStatus("cluster_e", 1);
			chain.SetBranchStatus("cluster_eta", 1);
			chain.SetBranchStatus("cluster_phi", 1);

			chain.SetBranchAddress("clsmult",&clsmult);
			chain.SetBranchAddress("cluster_e",cluster_e);
			chain.SetBranchAddress("cluster_eta",cluster_eta);
			chain.SetBranchAddress("cluster_phi",cluster_phi);
		} else {
			chain.SetBranchStatus("emcal_clsmult", 1);
			chain.SetBranchStatus("emcal_cluster_e", 1);
			chain.SetBranchStatus("emcal_cluster_eta", 1);
			chain.SetBranchStatus("emcal_cluster_phi", 1);

			chain.SetBranchAddress("emcal_clsmult",&clsmult);
			chain.SetBranchAddress("emcal_cluster_e",cluster_e);
			chain.SetBranchAddress("emcal_cluster_eta",cluster_eta);
			chain.SetBranchAddress("emcal_cluster_phi",cluster_phi);
		}

		// if applyCorr is set, then apply correction factor from MC Jet Calibration 
		TFile *corrFile;
	  	TF1 *correction = new TF1("jet energy correction","1",0,80);
	  	if(applyCorr) {
	      	corrFile = new TFile("JES_IsoCorr_NumInv.root","READ");
	      	corrFile -> cd();
	      	correction = (TF1*)corrFile -> Get("corrFit_Iso0");
	    }

		int eventnumber = 0; // number of events in tree (used for showing iteration through ttree)
		int events = 0; // number of events that pass event cuts (used for event level normalization)
	    
	    Long64_t nEntries = chain.GetEntries();
	    std::cout << nEntries << std::endl;

	    // main event loop 
	    for (Long64_t entry = 0; entry < nEntries; ++entry) {
	    //for (Long64_t entry = 0; entry < 1000000; ++entry) {
	        chain.GetEntry(entry);
	    	if (eventnumber % 10000 == 0) cout << "event " << eventnumber << endl;
	    	eventnumber++;

	    	// require jet trigger in data
	  		bool jettrig = false;
	  		for (auto t : *triggerVector) {
	  			if (t == 18 || (t == 34 && fabs(zvtx) < 10.0)) {
	  				jettrig = true;
	  				break;
	  			}
	  		}

	  		// require no negative energy jets (known jet background)
	  		bool negJet = false;
	  		for (int i = 0; i < nJet; i++) {
	  			if ((*e)[i] < 0) {
	  				negJet = true;
	  			}
	  		}

	  		// require at least 2 jets in event and z vertex < 30 cm 
	  		if (!jettrig && !sim) { continue; }
	  		if (isnan(zvtx)) { continue; }
	  		if (zvtx < -30 || zvtx > 30) { continue; }
	  		if (negJet) { continue; }
	  		if (nJet < 2) { continue; }		

	  		// find the leading and subleading jets of the event
	  		int ind_lead = 0;
	  		int ind_sub = 0;
	  		float temp_lead = 0;
	  		float temp_sub = 0;
	  		for (int i = 0; i < nJet; i++) {
	  			if ((*pt)[i] > temp_lead) {
	  				if (temp_lead != 0) {
	  					temp_sub = temp_lead;
	  					ind_sub = ind_lead;
	  				}
	  				temp_lead = (*pt)[i];
	  				ind_lead = i;
	  			} else if ((*pt)[i] > temp_sub) {
	  				temp_sub = (*pt)[i];
	  				ind_sub = i;
	  			}
	  		}

	  		// require leading and subleading jets be above pt threshold and in central eta range 
	  		if (fabs((*eta)[ind_lead]) > jetetamax || fabs((*eta)[ind_sub]) > jetetamax) { continue; }
	  		if ((*pt)[ind_lead] < lead_ptmin || (*pt)[ind_sub] < 0.3*(*pt)[ind_lead]) { continue; }		

	  		// create leading and subleading jet vectors and apply JES calibration correction if applicable 
	  		TVector3 lead, sub;
	  		if (applyCorr) {
	    		lead.SetPtEtaPhi((*pt)[ind_lead]*correction->Eval((*pt)[ind_lead]), (*eta)[ind_lead], (*phi)[ind_lead]);
	    		sub.SetPtEtaPhi((*pt)[ind_sub]*correction->Eval((*pt)[ind_sub]), (*eta)[ind_sub], (*phi)[ind_sub]);
	    	} else {
	    		lead.SetPtEtaPhi((*pt)[ind_lead], (*eta)[ind_lead], (*phi)[ind_lead]);
	    		sub.SetPtEtaPhi((*pt)[ind_sub], (*eta)[ind_sub], (*phi)[ind_sub]);
	    	}

	    	if (lead.Pt() < 15.0 || lead.Pt() > 20.0) { continue; }

	    	h_vz->Fill(zvtx);
	  		h_deltaphi->Fill(lead.DeltaPhi(sub));

	  		// require deltaphi > 2.75 (7pi/8)
	  		// transverse region is [pi/3,2pi/3] from leading jet 
	  		if (fabs(lead.DeltaPhi(sub)) > deltaphimin) {
	  			
	  			// fill jet QA histograms 
	  			h_pass_deltaphi->Fill(lead.DeltaPhi(sub));
	  			h_pass_xj->Fill(sub.Pt()/lead.Pt());
	  			h_pass_spectra->Fill(lead.Pt());
	  			h_pass_spectra->Fill(sub.Pt());
	  			h_leadjet->Fill(lead.Pt());
		  		h_subjet->Fill(sub.Pt());
		  		h_leadphi->Fill(lead.Phi());
		  		h_leadeta->Fill(lead.Eta());
		  		h_subphi->Fill(sub.Phi());
		  		h_subeta->Fill(sub.Eta());
	  			h_aj_ptavg->Fill((lead.Pt()+sub.Pt())/2.0, (lead.Pt()-sub.Pt())/(lead.Pt()+sub.Pt()));

	  			// define energy variables in the towards, transverse and away regions 
	  			float et_towards = 0;
	  			float et_transverse = 0;
	  			float et_away = 0;
	  			int ntopo_towards[] = {0,0,0,0,0,0,0,0};
	  			int ntopo_transverse[] = {0,0,0,0,0,0,0,0};
	  			int ntopo_away[] = {0,0,0,0,0,0,0,0};
	  			float sume_topo_towards[] = {0,0,0,0,0,0,0,0};
	  			float sume_topo_transverse[] = {0,0,0,0,0,0,0,0};
	  			float sume_topo_away[] = {0,0,0,0,0,0,0,0};

	  			if (!clusters) { // using towers to find total energy in towards, transverse and away regions 
		  			for (int i = 0; i < emcaln; i++) {
		  				TVector3 em;
		  				em.SetPtEtaPhi(emcale[i]/cosh(emcaleta[i]),emcaleta[i],emcalphi[i]); // define tower vector
		  				float dphi = lead.DeltaPhi(em); // find the deltaphi between leading jet and tower
		  				h_ue_2D_total->Fill(emcaleta[i],dphi,emcale[i]/cosh(emcaleta[i]));
		  				if (fabs(dphi) < M_PI/3.0) { // towards region 
		  					et_towards += emcale[i]/cosh(emcaleta[i]);
		  					h_ue_2D_towards->Fill(emcaleta[i],dphi,emcale[i]/cosh(emcaleta[i]));
		  				} else if (fabs(dphi) > M_PI/3.0 && fabs(dphi) < (2.0*M_PI)/3.0) { // transverse region 
							et_transverse += emcale[i]/cosh(emcaleta[i]);
		  					h_ue_2D_transverse->Fill(emcaleta[i],dphi,emcale[i]/cosh(emcaleta[i]));
		  					h_trans_et_2D_index_emcal->Fill(emcalieta[i],emcaliphi[i],emcale[i]/cosh(emcaleta[i]));
		  					h_trans_et_2D_index_total->Fill(emcalieta[i]/4,emcaliphi[i]/4,emcale[i]/cosh(emcaleta[i]));
		  				} else if (fabs(dphi) > (2.0*M_PI)/3.0) { // away region 
		  					et_away += emcale[i]/cosh(emcaleta[i]);
		  					h_ue_2D_away->Fill(emcaleta[i],dphi,emcale[i]/cosh(emcaleta[i]));
		  				}
		  			}

		  			for (int i = 0; i < ihcaln; i++) {
		  				TVector3 ih;
		  				ih.SetPtEtaPhi(ihcale[i]/cosh(ihcaleta[i]),ihcaleta[i],ihcalphi[i]);
		  				float dphi = lead.DeltaPhi(ih);
		  				h_ue_2D_total->Fill(ihcaleta[i],dphi,ihcale[i]/cosh(ihcaleta[i]));
		  				if (fabs(dphi) < M_PI/3.0) {
		  					et_towards += ihcale[i]/cosh(ihcaleta[i]);
		  					h_ue_2D_towards->Fill(ihcaleta[i],dphi,ihcale[i]/cosh(ihcaleta[i]));
		  				} else if (fabs(dphi) > M_PI/3.0 && fabs(dphi) < (2.0*M_PI)/3.0) {
							et_transverse += ihcale[i]/cosh(ihcaleta[i]);
		  					h_ue_2D_transverse->Fill(ihcaleta[i],dphi,ihcale[i]/cosh(ihcaleta[i]));
		  					h_trans_et_2D_index_ihcal->Fill(ihcalieta[i],ihcaliphi[i],ihcale[i]/cosh(ihcaleta[i]));
		  					h_trans_et_2D_index_total->Fill(ihcalieta[i],ihcaliphi[i],ihcale[i]/cosh(ihcaleta[i]));
		  				} else if (fabs(dphi) > (2.0*M_PI)/3.0) {
		  					et_away += ihcale[i]/cosh(ihcaleta[i]);
		  					h_ue_2D_away->Fill(ihcaleta[i],dphi,ihcale[i]/cosh(ihcaleta[i]));
		  				}
		  			}

		  			for (int i = 0; i < ohcaln; i++) {
		  				TVector3 oh;
		  				oh.SetPtEtaPhi(ohcale[i]/cosh(ohcaleta[i]),ohcaleta[i],ohcalphi[i]);
		  				float dphi = lead.DeltaPhi(oh);
		  				h_ue_2D_total->Fill(ohcaleta[i],dphi,ohcale[i]/cosh(ohcaleta[i]));
		  				if (fabs(dphi) < M_PI/3.0) {
		  					et_towards += ohcale[i]/cosh(ohcaleta[i]);
		  					h_ue_2D_towards->Fill(ohcaleta[i],dphi,ohcale[i]/cosh(ohcaleta[i]));
		  				} else if (fabs(dphi) > M_PI/3.0 && fabs(dphi) < (2.0*M_PI)/3.0) {
							et_transverse += ohcale[i]/cosh(ohcaleta[i]);
		  					h_ue_2D_transverse->Fill(ohcaleta[i],dphi,ohcale[i]/cosh(ohcaleta[i]));
		  					h_trans_et_2D_index_ohcal->Fill(ohcalieta[i],ohcaliphi[i],ohcale[i]/cosh(ohcaleta[i]));
		  					h_trans_et_2D_index_total->Fill(ohcalieta[i],ohcaliphi[i],ohcale[i]/cosh(ohcaleta[i]));
		  				} else if (fabs(dphi) > (2.0*M_PI)/3.0) {
		  					et_away += ohcale[i]/cosh(ohcaleta[i]);
		  					h_ue_2D_away->Fill(ohcaleta[i],dphi,ohcale[i]/cosh(ohcaleta[i]));
		  				}
		  			}
		  		} else { // using clusters to find total energy in towards, transverse and away regions 
	  				for (int i = 0; i < clsmult; i++) {
		  				TVector3 cls;
		  				cls.SetPtEtaPhi(cluster_e[i]/cosh(cluster_eta[i]),cluster_eta[i],cluster_phi[i]); // define cluster vector 
		  				float dphi = lead.DeltaPhi(cls); // find the deltaphi between leading jet and cluster 
		  				h_ue_2D_total->Fill(cluster_eta[i],dphi,cluster_e[i]/cosh(cluster_eta[i]));
		  				if (fabs(dphi) < M_PI/3.0) { // towards region 
		  					et_towards += cluster_e[i]/cosh(cluster_eta[i]);
		  					for (int j = 0; j < 8; j++) {
		  						if (cluster_e[i] > float(topo_thresholds[j]/1000.0)) {
		  							ntopo_towards[j] += 1;
		  							sume_topo_towards[j] += cluster_e[i]/cosh(cluster_eta[i]);
		  							h_topo_towards[j]->Fill(cluster_e[i]/cosh(cluster_eta[i]));
		  							h_2D_topo_towards[j]->Fill(cluster_eta[i],dphi,cluster_e[i]/cosh(cluster_eta[i]));
		  						}
		  					}
		  					h_ue_2D_towards->Fill(cluster_eta[i],dphi,cluster_e[i]/cosh(cluster_eta[i]));
		  				} else if (fabs(dphi) > M_PI/3.0 && fabs(dphi) < (2.0*M_PI)/3.0) { // transverse region 
							et_transverse += cluster_e[i]/cosh(cluster_eta[i]);
		  					for (int j = 0; j < 8; j++) {
		  						if (cluster_e[i] > float(topo_thresholds[j]/1000.0)) {
		  							ntopo_transverse[j] += 1;
		  							sume_topo_transverse[j] += cluster_e[i]/cosh(cluster_eta[i]);
		  							h_topo_transverse[j]->Fill(cluster_e[i]/cosh(cluster_eta[i]));
		  							h_2D_topo_transverse[j]->Fill(cluster_eta[i],dphi,cluster_e[i]/cosh(cluster_eta[i]));
		  						}
		  					}
		  					h_ue_2D_transverse->Fill(cluster_eta[i],dphi,cluster_e[i]/cosh(cluster_eta[i]));
		  				} else if (fabs(dphi) > (2.0*M_PI)/3.0) { // away region 
		  					et_away += cluster_e[i]/cosh(cluster_eta[i]);
		  					for (int j = 0; j < 8; j++) {
		  						if (cluster_e[i] > float(topo_thresholds[j]/1000.0)) {
		  							ntopo_away[j] += 1;
		  							sume_topo_away[j] += cluster_e[i]/cosh(cluster_eta[i]);
		  							h_topo_away[j]->Fill(cluster_e[i]/cosh(cluster_eta[i]));
		  							h_2D_topo_away[j]->Fill(cluster_eta[i],dphi,cluster_e[i]/cosh(cluster_eta[i]));
		  						}
		  					}
		  					h_ue_2D_away->Fill(cluster_eta[i],dphi,cluster_e[i]/cosh(cluster_eta[i]));
		  				}
		  			}
	  			}

	  			h_et_towards->Fill(et_towards);
	  			h_et_transverse->Fill(et_transverse);
	  			h_et_away->Fill(et_away);
	  			h_ue_towards->Fill(et_towards/(secteta*sectphi));
	  			h_ue_transverse->Fill(et_transverse/(secteta*sectphi));
	  			h_ue_away->Fill(et_away/(secteta*sectphi));

	  			for (int i = 0; i < 8; i++) {
	  				h_ntopo_towards[i]->Fill(ntopo_towards[i]);
	  				h_ntopo_transverse[i]->Fill(ntopo_transverse[i]);
	  				h_ntopo_away[i]->Fill(ntopo_away[i]);
	  				h_sume_topo_towards[i]->Fill(sume_topo_towards[i]);
	  				h_sume_topo_transverse[i]->Fill(sume_topo_transverse[i]);
	  				h_sume_topo_away[i]->Fill(sume_topo_away[i]);
	  			}

	  			h_ue_xj_towards->Fill(sub.Pt()/lead.Pt(),et_towards);
	  			h_ue_xj_transverse->Fill(sub.Pt()/lead.Pt(),et_transverse);
	  			h_ue_xj_away->Fill(sub.Pt()/lead.Pt(),et_away);
	  			h_ue_pt_towards->Fill(lead.Pt(),et_towards);
	  			h_ue_pt_transverse->Fill(lead.Pt(),et_transverse);
	  			h_ue_pt_away->Fill(lead.Pt(),et_away);
	  			h_et_pt_towards_2D->Fill(lead.Pt(),et_towards);
	  			h_et_pt_transverse_2D->Fill(lead.Pt(),et_transverse);
	  			h_et_pt_away_2D->Fill(lead.Pt(),et_away);

	  			events++;
	  		}

	  	}

	  	h_ue_2D_total->Scale(1.0/(events*deltaeta*deltaphi));
	  	h_ue_2D_towards->Scale(1.0/(events*deltaeta*deltaphi));
	  	h_ue_2D_transverse->Scale(1.0/(events*deltaeta*deltaphi));
	  	h_ue_2D_away->Scale(1.0/(events*deltaeta*deltaphi));

	  	for (int i = 0; i < 6; i++) {
	  		h_2D_topo_towards[i]->Scale(1.0/(events*deltaeta*deltaphi));
	  		h_2D_topo_transverse[i]->Scale(1.0/(events*deltaeta*deltaphi));
	  		h_2D_topo_away[i]->Scale(1.0/(events*deltaeta*deltaphi));
	  	}

	  	h_ue_xj_towards->Scale(1.0/(secteta*sectphi));
	  	h_ue_xj_transverse->Scale(1.0/(secteta*sectphi));
	  	h_ue_xj_away->Scale(1.0/(secteta*sectphi));

	  	h_ue_pt_towards->Scale(1.0/(secteta*sectphi));
	  	h_ue_pt_transverse->Scale(1.0/(secteta*sectphi));
	  	h_ue_pt_away->Scale(1.0/(secteta*sectphi));

	  	out->Write();
	  	out->Close();
  	}
}
