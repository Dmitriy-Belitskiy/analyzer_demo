#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/global/EDAnalyzer.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Utilities/interface/Span.h"

// root include files
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TDirectory.h"

// L1 scouting
#include "DataFormats/L1Scouting/interface/L1ScoutingMuon.h"
#include "DataFormats/L1Scouting/interface/L1ScoutingCalo.h"
#include "DataFormats/L1Scouting/interface/OrbitCollection.h"
#include "L1TriggerScouting/Utilities/interface/conversion.h"
#include "L1TriggerScouting/Utilities/interface/convertToL1TFormat.h"

// l1trigger
#include "DataFormats/L1Trigger/interface/Muon.h"
#include "DataFormats/L1Trigger/interface/Jet.h"
#include "DataFormats/L1Trigger/interface/EGamma.h"
#include "DataFormats/L1Trigger/interface/Tau.h"
#include "DataFormats/L1Trigger/interface/EtSum.h"

#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include <map>
#include <string>


#include <array>
#include <fstream>
#include <cstdint>

#include <typeinfo>

using namespace l1ScoutingRun3;

class DemoAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit DemoAnalyzer(const edm::ParameterSet&);
  ~DemoAnalyzer() {}
  static void fillDescriptions(edm::ConfigurationDescriptions&);


private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void beginJob() override;
  void endJob() override;


  void processDataBx(
    unsigned bx,
    const edm::Handle<MuonOrbitCollection>& muonsCollection,
    const edm::Handle<JetOrbitCollection>& jetsCollection,
    const edm::Handle<EGammaOrbitCollection>& eGammasCollection,
    const edm::Handle<TauOrbitCollection>& tausCollection,
    const edm::Handle<BxSumsOrbitCollection>& bxSumsCollection
  );

  // tokens for scouting data
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::Muon>> muonsTokenData_;
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::Jet>> jetsTokenData_;
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::EGamma>> eGammasTokenData_;
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::Tau>> tausTokenData_;
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::BxSums>> bxSumsTokenData_;




  // l1t standard data format
  std::vector<l1t::Jet> l1jets_;
  std::vector<l1t::EGamma> l1egs_;
  std::vector<l1t::EtSum> l1sums_;
  std::vector<l1t::Tau> l1taus_;
  std::vector<l1t::Muon> l1muons_;

  // map containing TH2D histograms
  std::map<std::string, TH2D*> m_2dhist_;

  // tree for lumisections
  unsigned lumisection_;

  // map containing objects
  std::map<int, std::vector<float>> muons_n;
  std::map<int, std::vector<float>> jets_n;
  std::map<int, std::vector<float>> eGammas_n;
  std::map<int, std::vector<float>> esum_n;
  std::map<int, std::vector<float>> taus_n;
  edm::Service<TFileService> fs;
  TFileDirectory histoSubDir = fs->mkdir("histograms");
};


DemoAnalyzer::DemoAnalyzer(const edm::ParameterSet& iPSet)
  : muonsTokenData_(consumes(iPSet.getParameter<edm::InputTag>("muonsTag"))),
    jetsTokenData_(consumes(iPSet.getParameter<edm::InputTag>("jetsTag"))),
    eGammasTokenData_(consumes(iPSet.getParameter<edm::InputTag>("eGammasTag"))),
    tausTokenData_(consumes(iPSet.getParameter<edm::InputTag>("tausTag"))),
    bxSumsTokenData_(consumes(iPSet.getParameter<edm::InputTag>("bxSumsTag")))
  {

  // test shared resources
  usesResource(TFileService::kSharedResource);

  // init internal containers for l1 objects
  l1muons_.reserve(8);
  l1jets_.reserve(12);
  l1egs_.reserve(12);
  l1taus_.reserve(12);
  l1sums_.reserve(12);

   
}



void DemoAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
  edm::Handle<OrbitCollection<l1ScoutingRun3::Muon>> muonsCollection;
  edm::Handle<OrbitCollection<l1ScoutingRun3::Jet>> jetsCollection;
  edm::Handle<OrbitCollection<l1ScoutingRun3::EGamma>> eGammasCollection;
  edm::Handle<OrbitCollection<l1ScoutingRun3::Tau>> tausCollection;
  edm::Handle<OrbitCollection<l1ScoutingRun3::BxSums>> bxSumsCollection;

  iEvent.getByToken(muonsTokenData_, muonsCollection);
  iEvent.getByToken(jetsTokenData_, jetsCollection);
  iEvent.getByToken(eGammasTokenData_, eGammasCollection);
  iEvent.getByToken(tausTokenData_, tausCollection);
  iEvent.getByToken(bxSumsTokenData_, bxSumsCollection);


  auto lumi = iEvent.luminosityBlock();

  auto a5 = iEvent.id().event();

  uint64_t a1 = (a5 >> (6 + 12)) & ((1ULL << 46) - 1);  // top 46 bits
  uint64_t a2 = (a5 >> 12)       & 0x3F;                // next 6 bits
  uint64_t a3 =  a5              & 0xFFF;               // last 12 bits

  lumisection_ = ( (a1<<6) | a2 );


  // process all BX 
  for (unsigned bx = 0; bx < 3564; ++bx) {
    processDataBx(
        bx,
        muonsCollection,
        jetsCollection,
        eGammasCollection,
        tausCollection,
        bxSumsCollection
    );
  }

 }

void DemoAnalyzer::processDataBx(
    unsigned bx,
    const edm::Handle<MuonOrbitCollection>& muonsCollection,
    const edm::Handle<JetOrbitCollection>& jetsCollection,
    const edm::Handle<EGammaOrbitCollection>& eGammasCollection,
    const edm::Handle<TauOrbitCollection>& tausCollection,
    const edm::Handle<BxSumsOrbitCollection>& bxSumsCollection
  ) {

    // get iterator for the current BX
    const auto& jets = jetsCollection->bxIterator(bx);
    const auto& eGammas = eGammasCollection->bxIterator(bx);
    const auto& taus = tausCollection->bxIterator(bx);
    const auto& bxSums = bxSumsCollection->bxIterator(bx);
    const auto& muons = muonsCollection->bxIterator(bx);

    //std::cout << "bx=" << bx << std::endl;
    // convert scouting objects to l1t::objects for semplicity
    // Note: Scouting objects are stored in hw quantities, if only a subset
    // of the features is needed, the function in L1TriggerScouting/Utilities/interface/conversion.h
    // can be used to get physical quantities.
    // for example, the momentum of a muon can be obtained with ugmt::fPt(hwPt);
    l1jets_.clear();
    l1egs_.clear();
    l1taus_.clear();
    l1sums_.clear();
    l1muons_.clear();


    for (const auto& muon : muons) {
      l1muons_.emplace_back(getL1TMuon(muon));
    }
    for (const auto& jet : jets) {
      l1jets_.emplace_back(getL1TJet(jet));
    }
    for (const auto& egamma : eGammas) {
      l1egs_.emplace_back(getL1TEGamma(egamma));
    }
    for (const auto& tau : taus) {
      l1taus_.emplace_back(getL1TTau(tau));
    }


    // // muons
    if (muons_n.find(lumisection_) == muons_n.end()) {
      muons_n[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (size_t i = 0; i < l1muons_.size(); ++i) {
      muons_n[lumisection_][bx] += 1;
    }



 // jets
    if (jets_n.find(lumisection_) == jets_n.end()) {
      jets_n[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (const auto& jet: l1jets_){
      jets_n[lumisection_][bx] += 1;
    }



     // eGammas
    if (eGammas_n.find(lumisection_) == eGammas_n.end()) {
      eGammas_n[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (size_t i = 0; i < l1egs_.size(); ++i) {
      eGammas_n[lumisection_][bx] += 1;
    }


    // taus
    if (taus_n.find(lumisection_) == taus_n.end()) {
      taus_n[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (size_t i = 0; i < l1taus_.size(); ++i) {
      taus_n[lumisection_][bx] += 1;
    }

    // esum
    if (bxSums.size()>0) {

      l1t::EtSum l1sum;

      if (esum_n.find(lumisection_) == esum_n.end()) {
        esum_n[lumisection_] = std::vector<float>(3564, 0.0f);
      }

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTotalEt);
      float pt_value = static_cast<float>(l1sum.pt());
      esum_n[lumisection_][bx] += pt_value;

    }

}

void DemoAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

// ------------ method called once each job just before starting event loop  ------------
void DemoAnalyzer::beginJob() {
}


// ------------ method called once each job just after ending the event loop  ------------
void DemoAnalyzer::endJob() {

  // fill histograms
  const int nBX = 3564;

  int minLS = std::numeric_limits<int>::max();
  int maxLS = std::numeric_limits<int>::min();

  // muons
  for (const auto& [key, _] : muons_n) {
      if (key < minLS) minLS = key;
      if (key > maxLS) maxLS = key;
  }

  const int nLumiBins = maxLS - minLS + 1;

  m_2dhist_["MuonBxOcc2D"] = histoSubDir.make<TH2D>( "MuonBxOcc2D", "Muon per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : muons_n) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["MuonBxOcc2D"]->Fill(key, bx, values[bx]);
    }
  }

  muons_n.clear();


   // jets
  m_2dhist_["JetBxOcc2D_n"] = histoSubDir.make<TH2D>( "JetBxOcc2D_n", "Jet per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : jets_n) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["JetBxOcc2D_n"]->Fill(key, bx, values[bx]);
    }
  }

  jets_n.clear();


  // eGammas


      m_2dhist_["eGammaBxOcc2D_n"] = histoSubDir.make<TH2D>( "eGammaBxOcc2D_n", "eGamma per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : eGammas_n) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["eGammaBxOcc2D_n"]->Fill(key, bx, values[bx]);
    }
  }

  eGammas_n.clear();


  // taus
  m_2dhist_["tauBxOcc2D"] = histoSubDir.make<TH2D>( "tauBxOcc2D", "tau per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : taus_n) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["tauBxOcc2D"]->Fill(key, bx, values[bx]);
    }
  }

  taus_n.clear();


  // Energy sum
  m_2dhist_["energySum2D"] = histoSubDir.make<TH2D>( "energySum2D", "energy per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : esum_n) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["energySum2D"]->Fill(key, bx, values[bx]);
    }
  }

  esum_n.clear();

}

DEFINE_FWK_MODULE(DemoAnalyzer);
