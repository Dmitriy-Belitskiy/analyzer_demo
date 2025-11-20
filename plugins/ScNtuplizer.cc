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
#include "TTree.h"
#include "TFile.h"
#include "TBranch.h"
#include "TDirectory.h"

// scouting 
#include "DataFormats/L1Scouting/interface/L1ScoutingMuon.h"
#include "DataFormats/L1Scouting/interface/L1ScoutingCalo.h"
#include "DataFormats/L1Scouting/interface/OrbitCollection.h"
#include "L1TriggerScouting/Utilities/interface/conversion.h"
#include "L1TriggerScouting/Utilities/interface/convertToL1TFormat.h"

// l1trigger
#include "DataFormats/L1Trigger/interface/BXVector.h"
#include "DataFormats/L1Trigger/interface/Muon.h"
#include "DataFormats/L1Trigger/interface/Jet.h"
#include "DataFormats/L1Trigger/interface/EGamma.h"
#include "DataFormats/L1Trigger/interface/Tau.h"
#include "DataFormats/L1Trigger/interface/EtSum.h"


#include <memory>
#include <utility>
#include <vector>
#include <set>
#include <iostream>

using namespace l1ScoutingRun3;

class ScNtuplizer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit ScNtuplizer(const edm::ParameterSet&);
  ~ScNtuplizer() {}
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
    const edm::Handle<BxSumsOrbitCollection>& bxSumsCollection
  );

  // store filled bx in a vector
  void getFilledBx(
    const edm::Handle<MuonOrbitCollection>& muonsCollection,
    const edm::Handle<JetOrbitCollection>& jetsCollection,
    const edm::Handle<EGammaOrbitCollection>& eGammasCollection,
    const edm::Handle<BxSumsOrbitCollection>& bxSumsCollection,
    std::vector<unsigned>& filledBxVec
  );

  void resetTreeBranches();

  // tokens for scouting data
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::Muon>> muonsTokenData_;
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::Jet>> jetsTokenData_;
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::EGamma>> eGammasTokenData_;
  edm::EDGetTokenT<OrbitCollection<l1ScoutingRun3::BxSums>> bxSumsTokenData_;
  
  edm::EDGetTokenT<std::vector<unsigned>> selectedBxToken_;

  // Map containing different input tags
  std::map<std::string, edm::InputTag> m_inputTags;

  // process BX selected online
  bool onlineSelection_;

  // the root file service to handle the output file
  edm::Service<TFileService> fs;

  TTree* eventsTree_;

  unsigned orbitNum_;
  unsigned bx_;
  // jet
  std::vector<float> jetEt_, jetPhi_, jetEta_;
  int nJets_;
  // eg
  std::vector<float> egEt_, egPhi_, egEta_;
  std::vector<int> egIso_;
  int nEGammas_;
  // etSums
  float totalEt_, totalHt_, missEt_, missEtPhi_, missHt_, missHtPhi_;
  int towerCount_; 
  //muons
  std::vector<float> muonPt_, muonPhie_, muonEtae_;
  std::vector<float> muonPtu_, muonPhi_, muonEta_;
  std::vector<int> tfIndex_, dxy_, charge_, qual_;
  int nMuons_;
};

ScNtuplizer::ScNtuplizer(const edm::ParameterSet& iPSet){
  // get the input tags
  m_inputTags["muonsTag_"] = iPSet.getParameter<edm::InputTag>("muonsTag");
  m_inputTags["jetsTag_"] = iPSet.getParameter<edm::InputTag>("jetsTag");
  m_inputTags["eGammasTag_"] = iPSet.getParameter<edm::InputTag>("eGammasTag");
  m_inputTags["bxSumsTag_"] = iPSet.getParameter<edm::InputTag>("bxSumsTag");
  
  onlineSelection_ = iPSet.getUntrackedParameter<bool>("onlineSelection", false);

  // analyzing online selection stream, pass selected bx Tag
  if (onlineSelection_){
    m_inputTags["selectedBxTag_"] = iPSet.getParameter<edm::InputTag>("selectedBxTag");
    selectedBxToken_ = consumes<std::vector<unsigned>>(m_inputTags["selectedBxTag_"]);
  }

  // get tokens
  muonsTokenData_ = consumes<OrbitCollection<l1ScoutingRun3::Muon>>(m_inputTags["muonsTag_"]);
  jetsTokenData_ = consumes<OrbitCollection<l1ScoutingRun3::Jet>>(m_inputTags["jetsTag_"]);
  eGammasTokenData_ = consumes<OrbitCollection<l1ScoutingRun3::EGamma>>(m_inputTags["eGammasTag_"]);
  bxSumsTokenData_ = consumes<OrbitCollection<l1ScoutingRun3::BxSums>>(m_inputTags["bxSumsTag_"]);
  
  // create TTree
  eventsTree_ = fs->make<TTree>("Events", "Events");

  eventsTree_->Branch("orbit", &orbitNum_);
  eventsTree_->Branch("bx", &bx_);

  // jet
  eventsTree_->Branch("nJets", &nJets_);
  eventsTree_->Branch("jetEt", &jetEt_);
  eventsTree_->Branch("jetEta", &jetEta_);
  eventsTree_->Branch("jetPhi", &jetPhi_);

  // eg
  eventsTree_->Branch("nEGammas", &nEGammas_);
  eventsTree_->Branch("egEt", &egEt_);
  eventsTree_->Branch("egEta", &egEta_);
  eventsTree_->Branch("egPhi", &egPhi_);
  eventsTree_->Branch("egIso", &egIso_);

  // sums
  eventsTree_->Branch("etSum", &totalEt_);
  eventsTree_->Branch("htSum", &totalHt_);
  eventsTree_->Branch("etMiss", &missEt_);
  eventsTree_->Branch("etMissPhi", &missEtPhi_);
  eventsTree_->Branch("htMiss", &missHt_);
  eventsTree_->Branch("htMissPhi", &missHtPhi_);
  eventsTree_->Branch("towerCounts", &towerCount_);

  // muons
  eventsTree_->Branch("nMuons", &nMuons_);
  eventsTree_->Branch("muonPt", &muonPt_);
  eventsTree_->Branch("muonPhi", &muonPhi_ );
  eventsTree_->Branch("muonEta", &muonEta_);
  eventsTree_->Branch("muonPhiAtVtx", &muonPhie_ );
  eventsTree_->Branch("muonEtaAtVtx", &muonEtae_);
  eventsTree_->Branch("muonCharge", &charge_);
  eventsTree_->Branch("muonTfIndex", &tfIndex_);
  eventsTree_->Branch("muonQual", &qual_);
  eventsTree_->Branch("muonDxy", &dxy_);
  eventsTree_->Branch("muonPtu", &muonPtu_);

  resetTreeBranches();
}

void ScNtuplizer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
  edm::Handle<OrbitCollection<l1ScoutingRun3::Muon>> muonsCollection; 
  edm::Handle<OrbitCollection<l1ScoutingRun3::Jet>> jetsCollection; 
  edm::Handle<OrbitCollection<l1ScoutingRun3::EGamma>> eGammasCollection; 
  edm::Handle<OrbitCollection<l1ScoutingRun3::BxSums>> bxSumsCollection;

  
  iEvent.getByToken(muonsTokenData_, muonsCollection); 
  iEvent.getByToken(jetsTokenData_, jetsCollection); 
  iEvent.getByToken(eGammasTokenData_, eGammasCollection); 
  iEvent.getByToken(bxSumsTokenData_, bxSumsCollection); 
  
  std::vector<unsigned> bxList;
  if (onlineSelection_){
    edm::Handle<std::vector<unsigned>> selectedBx;
    iEvent.getByToken(selectedBxToken_, selectedBx);
    bxList = *selectedBx;
  } else {
    // get list of filled bx, i.e. with at least (>0 Muon) OR (>0 Jet) OR (>0 EGamma)
    // and store it in bxList vector
    getFilledBx(
      muonsCollection,
      jetsCollection,
      eGammasCollection,
      bxSumsCollection,
      bxList
    );
  }

  // process l1scouting orbit
  orbitNum_ = iEvent.id().event();

  for (const unsigned& bx: bxList) {
    processDataBx(
        bx,
        muonsCollection,
        jetsCollection,
        eGammasCollection,
        bxSumsCollection
      );
  } // end orbit loop
} 

void ScNtuplizer::processDataBx(
    unsigned bx,
    const edm::Handle<MuonOrbitCollection>& muonsCollection,
    const edm::Handle<JetOrbitCollection>& jetsCollection,
    const edm::Handle<EGammaOrbitCollection>& eGammasCollection,
    const edm::Handle<BxSumsOrbitCollection>& bxSumsCollection
  ) {

    resetTreeBranches();

    // store current BX
    bx_ = bx;
    
    // get objects for current BX
    const auto& jets = jetsCollection->bxIterator(bx);
    const auto& eGammas = eGammasCollection->bxIterator(bx);
    const auto& bxSums = bxSumsCollection->bxIterator(bx); // [0] only one valid sum.
    const auto& muons = muonsCollection->bxIterator(bx);
    
    // fill muon branches
    for (const auto& muon : muons) {
      l1t::Muon l1muon = getL1TMuon(muon);

      muonPt_.push_back(l1muon.pt());
      muonPhi_.push_back(float(l1muon.phi()));
      muonEta_.push_back(float(l1muon.eta()));
      muonPhie_.push_back(float(l1muon.phiAtVtx()));
      muonEtae_.push_back(float(l1muon.etaAtVtx()));
      charge_.push_back(l1muon.hwCharge());
      tfIndex_.push_back(l1muon.tfMuonIndex());
      qual_.push_back(l1muon.hwQual());
      dxy_.push_back(l1muon.hwDXY());
      muonPtu_.push_back(float(l1muon.ptUnconstrained()));
      
      nMuons_++;
    }

    // fill jet branches
    for (const auto& jet : jets) {
      l1t::Jet l1jet = getL1TJet(jet);
      
      jetEt_.push_back(l1jet.pt());
      jetPhi_.push_back(l1jet.phi());
      jetEta_.push_back(l1jet.eta());
      
      nJets_ ++;
    }

    // fill egamma branches
    for (const auto& egamma : eGammas) {
      l1t::EGamma l1eg = getL1TEGamma(egamma);

      egEt_.push_back(l1eg.pt());
      egPhi_.push_back(l1eg.phi());
      egEta_.push_back(l1eg.eta());
      egIso_.push_back(l1eg.hwIso());

      nEGammas_++;
    }
    
    // get subset of energy sums (if present, stored only if there is at least a calo object)
    if (bxSums.size()>0){
      l1t::EtSum l1sum;

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTotalEt);
      totalEt_=l1sum.pt();

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTotalHt);
      totalHt_=l1sum.pt();

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kMissingEt);
      missEt_=l1sum.pt();
      missEtPhi_=l1sum.phi();

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kMissingHt);
      missHt_=l1sum.pt();
      missHtPhi_=l1sum.phi();

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTowerCount);
      towerCount_=l1sum.hwPt();
    }
    
    // write event
    eventsTree_->Fill();
    
} // end bx

void ScNtuplizer::resetTreeBranches() {
  
  // jet
  nJets_=0;
  jetEt_.clear();
  jetPhi_.clear();
  jetEta_.clear();

  // eg
  nEGammas_=0;
  egEt_.clear();
  egPhi_.clear();
  egEta_.clear();
  egIso_.clear();

  // etSums
  totalEt_=0;
  totalHt_=0;
  missEt_=0;
  missEtPhi_=0;
  missHt_=0;
  missHtPhi_=0;
  towerCount_=0;

  // muons
  nMuons_=0;
  muonPt_.clear();
  muonPhi_.clear();
  muonEta_.clear();
  muonPhie_.clear();
  muonEtae_.clear();
  charge_.clear();
  tfIndex_.clear();
  qual_.clear();
  dxy_.clear();
  muonPtu_.clear();
}

void ScNtuplizer::getFilledBx(
    const edm::Handle<MuonOrbitCollection>& muonsCollection,
    const edm::Handle<JetOrbitCollection>& jetsCollection,
    const edm::Handle<EGammaOrbitCollection>& eGammasCollection,
    const edm::Handle<BxSumsOrbitCollection>& bxSumsCollection,
    std::vector<unsigned>& filledBxVec
  ) {
    // get filled bunch crossings for each collection
    std::set<unsigned> filledBx;
    
    for (const unsigned& bx : jetsCollection->getFilledBxs()) {
      filledBx.insert(bx);
    }

    for (const unsigned& bx : eGammasCollection->getFilledBxs()) {
      filledBx.insert(bx);
    }

    for (const unsigned& bx : muonsCollection->getFilledBxs()) {
      filledBx.insert(bx);
    }

    std::copy(filledBx.begin(), filledBx.end(), back_inserter(filledBxVec));
  }

void ScNtuplizer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}
 
// ------------ method called once each job just before starting event loop  ------------
void ScNtuplizer::beginJob() {
}

// ------------ method called once each job just after ending the event loop  ------------
void ScNtuplizer::endJob() {
}


DEFINE_FWK_MODULE(ScNtuplizer);
