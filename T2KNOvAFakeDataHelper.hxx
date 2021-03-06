#pragma once

#include "TDirectory.h"
#include "TFile.h"
#include "TH3.h"

#include <iostream>
#include <map>

namespace t2knova {

double GetFakeDataWeight_NOvAToT2K_PLep(int nu_pdg, int lep_pdg,
                                        double E_nu_GeV, double PLep_GeV,
                                        double EVisHadronic_GeV,
                                        bool interpolate);
double GetFakeDataWeight_NOvAToT2K_Q2(int nu_pdg, int lep_pdg, double E_nu_GeV,
                                      double Q2_GeV2, double EVisHadronic_GeV,
                                      bool interpolate);
double GetFakeDataWeight_NOvAToT2K_PtLep(int nu_pdg, int lep_pdg,
                                         double E_nu_GeV, double PtLep_GeV,
                                         double EVisHadronic_GeV,
                                         bool interpolate);
double GetFakeDataWeight_ND280ToNOvA(int nu_pdg, int lep_pdg, double E_nu_GeV,
                                     double PLep_GeV, double ThetaLep,
                                     int NFSCPi, int NFSPi0, int NOther,
                                     bool interpolate);

double GetFakeDataWeight_SKToNOvA(int nu_pdg, int lep_pdg, double E_nu_GeV,
                                  double PLep_GeV, double ThetaLep, int NFSCPi,
                                  int NFSPi0, int NOther, bool interpolate);
} // namespace t2knova

namespace t2knova {

TH1 *GetTH1(TFile *f, std::string const &name) {
  TDirectory *odir = gDirectory;

  TH1 *h;
  f->GetObject(name.c_str(), h);
  if (!h) {
    std::cout << "[ERROR]: Failed to find histogram named: " << name
              << std::endl;
  } else {
    h->SetDirectory(nullptr);
  }

  if (odir) {
    gDirectory->cd();
  }

  return h;
}

enum nuspecies { kNuMu = 0, kNuMub, kNuE, kNuEb };
const char *all_nuspecies[] = {"numu", "numub", "nue", "nueb"};

nuspecies getnuspec(int pdg) {
  switch (pdg) {
  case 14: {
    return kNuMu;
  }
  case -14: {
    return kNuMub;
  }
  case 12: {
    return kNuE;
  }
  case -12: {
    return kNuEb;
  }
  default: {
    std::cerr << "[ERROR]: Invalid neutrino PDG " << pdg
              << " passed to t2knova::getnuspec" << std::endl;
    abort();
  }
  }
}

enum reweightconfig {
  kT2KND_to_NOvA = 0,
  kSK_to_NOvA,
  kNOvA_to_T2KND_plep,
  kNOvA_to_T2KND_Q2,
  kNOvA_to_T2KND_ptlep,
  kNoWeight
};
const char *all_rwconfig[] = {"t2knd_to_nova", "sk_to_nova", "nova_to_t2k_plep",
                              "nova_to_t2k_Q2", "nova_to_t2k_ptlep"};

enum selection {
  kCCINC = 0,
  kCC0pi,
  kCC1cpi,
  kCC1pi0,
  kCCOther,
  kNCINC,
  kNC0pi,
  kNC1cpi,
  kNC1pi0,
  kNCOther
};
const char *all_sel[] = {"CCInc", "CC0Pi", "CC1CPi", "CC1Pi0", "CCOther",
                         "NCInc", "NC0Pi", "NC1CPi", "NC1Pi0", "NCOther"};

bool loaded = false;
std::map<nuspecies, std::map<reweightconfig, std::map<selection, TH3D *>>>
    rwhists;

void LoadHists() {
  TFile fin("FakeDataInputs.root");
  if (fin.IsZombie()) {
    std::cout << "Failed to read FakeDataInputs.root" << std::endl;
    abort();
  }
  int found = 0;
  for (int i = 0; i < 4; ++i) {
    nuspecies nuspec = nuspecies(i);
    std::string nuspec_str = all_nuspecies[i];

    for (int i = 0; i < 5; ++i) {
      reweightconfig rwconfig = reweightconfig(i);
      std::string rwconfig_str = all_rwconfig[i];

      for (int i = 0; i < 10; ++i) {
        selection sel = selection(i);
        std::string sel_str = all_sel[i];

        rwhists[nuspec][rwconfig][sel] = dynamic_cast<TH3D *>(
            t2knova::GetTH1(&fin, rwconfig_str + "_" + nuspec_str + "_" + sel_str));
        if (rwhists[nuspec][rwconfig][sel]) {
          rwhists[nuspec][rwconfig][sel]->SetDirectory(nullptr);
          found++;
        }
      }
    }
  }
  if (found == 0) {
    std::cout << "[ERROR]: Failed to find any histograms." << std::endl;
    abort();
  }
  loaded = true;
}

double EvalHist(TH3D *h, double x, double y, double z,
                bool interpolate = true) {

  int xbin = h->GetXaxis()->FindFixBin(x);
  int ybin = h->GetYaxis()->FindFixBin(y);
  int zbin = h->GetZaxis()->FindFixBin(z);

  if ((xbin != 0) && (xbin != (h->GetXaxis()->GetNbins() + 1)) && (ybin != 0) &&
      (ybin != (h->GetYaxis()->GetNbins() + 1)) && (zbin != 0) &&
      (zbin != (h->GetZaxis()->GetNbins() + 1))) {

    if (interpolate && (xbin > 1) && (xbin < h->GetXaxis()->GetNbins()) &&
        (ybin > 1) && (ybin < h->GetYaxis()->GetNbins()) && (zbin > 1) &&
        (zbin < h->GetZaxis()->GetNbins())) {
      return h->Interpolate(x, y, z);
    }
    return h->GetBinContent(xbin, ybin, zbin);
  }

  return 1;
}

double GetFakeDataWeight_NOvAToT2K_PLep(int nu_pdg, int lep_pdg,
                                        double E_nu_GeV, double PLep_GeV,
                                        double EVisHadronic_GeV,
                                        bool interpolate) {
  if (!loaded) {
    LoadHists();
  }
  nuspecies nuspec = getnuspec(nu_pdg);

  bool iscc = (nu_pdg != lep_pdg);

  TH3D *rathist = rwhists[nuspec][kNOvA_to_T2KND_plep][iscc ? kCCINC : kNCINC];
  if (rathist) {
    return EvalHist(rathist, E_nu_GeV, PLep_GeV, EVisHadronic_GeV, interpolate);
  }

  return 1;
}

double GetFakeDataWeight_NOvAToT2K_Q2(int nu_pdg, int lep_pdg, double E_nu_GeV,
                                      double Q2_GeV2, double EVisHadronic_GeV,
                                      bool interpolate) {
  if (!loaded) {
    LoadHists();
  }
  nuspecies nuspec = getnuspec(nu_pdg);

  bool iscc = (nu_pdg != lep_pdg);

  TH3D *rathist = rwhists[nuspec][kNOvA_to_T2KND_Q2][iscc ? kCCINC : kNCINC];
  if (rathist) {
    return EvalHist(rathist, E_nu_GeV, Q2_GeV2, EVisHadronic_GeV, interpolate);
  }
  return 1;
}

double GetFakeDataWeight_NOvAToT2K_PtLep(int nu_pdg, int lep_pdg,
                                         double E_nu_GeV, double PtLep_GeV,
                                         double EVisHadronic_GeV,
                                         bool interpolate) {
  if (!loaded) {
    LoadHists();
  }
  nuspecies nuspec = getnuspec(nu_pdg);

  bool iscc = (nu_pdg != lep_pdg);

  TH3D *rathist = rwhists[nuspec][kNOvA_to_T2KND_ptlep][iscc ? kCCINC : kNCINC];
  if (rathist) {
    return EvalHist(rathist, E_nu_GeV, PtLep_GeV, EVisHadronic_GeV,
                    interpolate);
  }
  return 1;
}

selection gett2ksel(bool iscc, int NFSCPi, int NFSPi0, int NOther) {
  selection sel = kCCINC;
  if ((NFSCPi + NFSPi0 + NOther) == 0) {
    sel = iscc ? kCC0pi : kNC0pi;
  } else if ((NFSCPi + NFSPi0 + NOther) == 1) {
    if (NFSCPi == 1) {
      sel = iscc ? kCC1cpi : kNC1cpi;
    } else if (NFSCPi == 1) {
      sel = iscc ? kCC1pi0 : kNC1pi0;
    } else {
      sel = iscc ? kCCOther : kNCOther;
    }
  } else {
    sel = iscc ? kCCOther : kNCOther;
  }
  return sel;
}

double GetFakeDataWeight_ND280ToNOvA(int nu_pdg, int lep_pdg, double E_nu_GeV,
                                     double PLep_GeV, double ThetaLep,
                                     int NFSCPi, int NFSPi0, int NOther,
                                     bool interpolate) {
  if (!loaded) {
    LoadHists();
  }
  nuspecies nuspec = getnuspec(nu_pdg);

  bool iscc = (nu_pdg != lep_pdg);

  selection sel = gett2ksel(iscc, NFSCPi, NFSPi0, NOther);

  TH3D *rathist = rwhists[nuspec][kT2KND_to_NOvA][sel];
  if (rathist) {
    return EvalHist(rathist, E_nu_GeV, PLep_GeV, ThetaLep, interpolate);
  }

  return 1;
}

double GetFakeDataWeight_SKToNOvA(int nu_pdg, int lep_pdg, double E_nu_GeV,
                                  double PLep_GeV, double ThetaLep, int NFSCPi,
                                  int NFSPi0, int NOther, bool interpolate) {
  if (!loaded) {
    LoadHists();
  }
  nuspecies nuspec = getnuspec(nu_pdg);

  bool iscc = (nu_pdg != lep_pdg);

  selection sel = gett2ksel(iscc, NFSCPi, NFSPi0, NOther);

  TH3D *rathist = rwhists[nuspec][kSK_to_NOvA][sel];
  if (rathist) {
    return EvalHist(rathist, E_nu_GeV, PLep_GeV, ThetaLep, interpolate);
  }

  return 1;
}

} // namespace t2knova