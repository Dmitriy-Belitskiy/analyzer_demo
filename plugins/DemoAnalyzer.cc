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

  unsigned get_counter(){
    return (counter);
  };

  void counter_advance(){
    counter++;
  }

  void reset_counter(){
    counter=0;
  }

  unsigned get_nb4(){
    unsigned nb4 = counter / 1634;
    return(nb4);

  }

  unsigned last_entry = 0;
  unsigned last_LS = 0;

  std::array<uint8_t, 3564> bx_mask_col;
  std::array<uint8_t, 3564> bx_mask_noncol;
  std::array<uint8_t, 3564> bx_mask_ag;

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void beginJob() override;
  void endJob() override;

  unsigned counter;


  void load_masks(){
    std::ifstream fin("bx_mask_ag.bin", std::ios::binary);
    if (!fin) throw std::runtime_error("Cannot open bx_mask.bin");

    fin.read(reinterpret_cast<char*>(bx_mask_ag.data()), bx_mask_ag.size());
    fin.close();

    fin.open("bx_mask_col.bin", std::ios::binary);
    if (!fin) throw std::runtime_error("Cannot open bx_mask.bin");

    fin.read(reinterpret_cast<char*>(bx_mask_col.data()), bx_mask_col.size());
    fin.close();


    fin.open("bx_mask_noncol.bin", std::ios::binary);
    if (!fin) throw std::runtime_error("Cannot open bx_mask.bin");

    fin.read(reinterpret_cast<char*>(bx_mask_noncol.data()), bx_mask_noncol.size());
    fin.close();

  }


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

  // map containing TH1D histograms
  std::map<std::string, TH1D*> m_1dhist_;

  // map containing TH2D histograms
  std::map<std::string, TH2D*> m_2dhist_;

  // tree for lumisections
  unsigned lumisection_;

  // map containing objects
  std::map<int, std::vector<float>> muons_b;
  std::map<int, std::vector<float>> jets_b;
  std::map<int, std::vector<float>> eGammas_b;
  std::map<int, std::vector<float>> taus_b;

  std::map<int, std::vector<float>> missing_b;
  std::map<int, std::vector<float>> esum_b;

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

  // Init histograms
  m_1dhist_["MuonBxOcc"] = histoSubDir.make<TH1D>("MuonBxOcc", "BX in orbit with at least one muon", 3566, -0.5, 3565.5);
  m_1dhist_["Jets"] = histoSubDir.make<TH1D>("Jets", "BX in orbit with number of jets", 3566, -0.5, 3565.5);
  m_1dhist_["eGammas"] = histoSubDir.make<TH1D>("eGammas", "BX in orbit with number of eGammas", 3566, -0.5, 3565.5);
  m_1dhist_["Taus"] = histoSubDir.make<TH1D>("Taus", "BX in orbit with number of Taus", 3566, -0.5, 3565.5);
  m_1dhist_["Etsums"] = histoSubDir.make<TH1D>("EtSums", "BX in orbit with number of total sums", 3566, -0.5, 3565.5);
  m_1dhist_["Htsums"] = histoSubDir.make<TH1D>("HtSums", "BX in orbit with number of HT sums", 3566, -0.5, 3565.5);
  m_1dhist_["MissingEt"] = histoSubDir.make<TH1D>("MissingEt", "BX in orbit with number of missing total sums", 3566, -0.5, 3565.5);
  m_1dhist_["MissingHt"] = histoSubDir.make<TH1D>("MissingHt", "BX in orbit with number of missing HT sums", 3566, -0.5, 3565.5);
  m_1dhist_["TowerCount"] = histoSubDir.make<TH1D>("TowerCount", "BX in orbit with number of Towercounts", 3566, -0.5, 3565.5);

  m_1dhist_["muonPt_c"] = histoSubDir.make<TH1D>("MPt_col", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  m_1dhist_["muonPt_nc"] = histoSubDir.make<TH1D>("MPt_ncol", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  m_1dhist_["muonPt_ag"] = histoSubDir.make<TH1D>("MPt_ag", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);

  // m_1dhist_["muonPt_c"] = histoSubDir.make<TH1D>("MPt_col", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  // m_1dhist_["muonPt_nc"] = histoSubDir.make<TH1D>("MPt_ncol", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  // m_1dhist_["muonPt_ag"] = histoSubDir.make<TH1D>("MPt_ag", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);

  m_1dhist_["jetEt_c"] = histoSubDir.make<TH1D>("JEt_col", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  m_1dhist_["jetEt_nc"] = histoSubDir.make<TH1D>("JEt_ncol", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  m_1dhist_["jetEt_ag"] = histoSubDir.make<TH1D>("JEt_ag", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);

  m_1dhist_["egEt_c"] = histoSubDir.make<TH1D>("eEt_col", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  m_1dhist_["egEt_nc"] = histoSubDir.make<TH1D>("eEt_ncol", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);
  m_1dhist_["egEt_ag"] = histoSubDir.make<TH1D>("eEt_ag", "BX in orbit with number of missing total sums", 200, -0.5, 200.5);


  m_1dhist_["muonPhi_c"] = histoSubDir.make<TH1D>("MPhi_col", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);
  m_1dhist_["muonPhi_nc"] = histoSubDir.make<TH1D>("MPhi_ncol", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);
  m_1dhist_["muonPhi_ag"] = histoSubDir.make<TH1D>("MPhi_ag", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);

  m_1dhist_["jetPhi_c"] = histoSubDir.make<TH1D>("JPhi_col", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);
  m_1dhist_["jetPhi_nc"] = histoSubDir.make<TH1D>("JPhi_ncol", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);
  m_1dhist_["jetPhi_ag"] = histoSubDir.make<TH1D>("JPhi_ag", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);

  m_1dhist_["egPhi_c"] = histoSubDir.make<TH1D>("ePhi_col", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);
  m_1dhist_["egPhi_nc"] = histoSubDir.make<TH1D>("ePhi_ncol", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);
  m_1dhist_["egPhi_ag"] = histoSubDir.make<TH1D>("ePhi_ag", "BX in orbit with number of missing total sums",200 , -3.14, 3.14);

  /*
  m_1dhist_["jetPt"] = histoSubDir.make<TH1D>("JPt_col", "BX in orbit with number of missing HT sums", 200, -0.5, 200.5);
  m_1dhist_["egammaPt"] = histoSubDir.make<TH1D>("EPt_col", "BX in orbit with number of Towercounts", 200, -0.5, 200.5);*/
  //init counter
  reset_counter();
  load_masks();


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

  // store lumisection number

  auto lumi = iEvent.luminosityBlock();

  //std::cout << lumi <<std::endl;


  //lumisection_ = ( (lumi<<4) | get_nb4() );
  //counter_advance();


  // for(int i = 0 ; i<3564;i++ ){
  //
  //       std::cout<<int(bx_mask_col[i])<<int(bx_mask_noncol[i])<<int(bx_mask_ag[i])<<std::endl;
  //   }


  //std::cout<<lumisection_<<std::endl ;
  //std::cout<<(lumi<<4)<<std::endl;
  // // --- Retrieve event info ---
  // auto bx_event = iEvent.id();
  // unsigned long long orbit = bx_event.event();
  // unsigned int lumi = iEvent.luminosityBlock();
  //
  auto a5 = iEvent.id().event();

  uint64_t a1 = (a5 >> (6 + 12)) & ((1ULL << 46) - 1);  // top 46 bits
  uint64_t a2 = (a5 >> 12)       & 0x3F;                // next 6 bits
  uint64_t a3 =  a5              & 0xFFF;               // last 12 bits

  // if (last_entry != a2){
  //   counter_advance();
  // }
  //
  //
  // if (last_entry != a2){
  //   counter_advance();
  // }
  //


  //last_entry = a2;

  //unsigned nb1 =get_counter()-1;

  //std::cout << get_counter()-1 << std::endl;

  //lumisection_ = get_counter()-1;

  //std::cout<< a1 << ", " << a2 << ", " << a3 << "\n";
  //std::cout<<a5<<std::endl;
  //std::cout<<( (a1<<6) | a2 )<<std::endl;
  lumisection_ = ( (a1<<6) | a2 );

  //std::cout<<typeid(a5).name()<<std::endl;


  // // --- Constants for CMS orbit structure ---
  // constexpr unsigned int ORBITS_PER_LS = 262144;
  // constexpr unsigned int ORBITS_PER_NB4 = ORBITS_PER_LS / 16; // 16384
  //
  // // --- Persistent reference per LS ---
  // static unsigned long long referenceOrbit = 0;
  // static unsigned int referenceLumi = 0;
  //
  // // --- Reset when new LS starts ---
  // if (referenceLumi != lumi) {
  //     referenceLumi = lumi;
  //     referenceOrbit = orbit;
  // }
  //
  // // --- Orbit offset inside LS ---
  // unsigned long long deltaOrbit = orbit - referenceOrbit;
  //
  // // --- Compute nb4 index (0–15) ---
  // unsigned int nb4_index = deltaOrbit / ORBITS_PER_NB4;
  // if (nb4_index > 15) nb4_index = 15;
  //
  // // --- Combined absolute fine-grained Lumi ID ---
  // unsigned int lumiNb4ID = lumi * 100 + nb4_index;

  // // --- Output / store result ---
  // std::cout << "Event: " << bx_event.event()
  //           << " | orbit≈ " << orbit
  //           << " | LS: " << lumi
  //           << " | nb4: " << nb4_index
  //           << " | LumiNb4ID: " << lumiNb4ID
  //           << std::endl;

 // --- Output / store result ---
 // std::cout << lumi  << std::endl;

  // process all BX in orbit containing at least a Muon
  // getFilledBxs() returns the list of filled BX in the muon orbit collection
  for (const unsigned& bx : muonsCollection->getFilledBxs()) {
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

    lumisection_ = lumisection_;

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

    // store some of the sums and make histograms
    if (bxSums.size()>0){
      l1t::EtSum l1sum;

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTotalEt);
      m_1dhist_["Etsums"]->Fill(bx, l1sum.pt());

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTotalHt);
      m_1dhist_["Htsums"]->Fill(bx, l1sum.pt());

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kMissingEt);
      m_1dhist_["MissingEt"]->Fill(bx, l1sum.pt());

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kMissingHt);
      m_1dhist_["MissingHt"]->Fill(bx, l1sum.pt());

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTowerCount);
      m_1dhist_["TowerCount"]->Fill(bx, l1sum.pt());
    }

    // fill histograms
    for (const auto& muon: l1muons_){
      m_1dhist_["MuonBxOcc"]->Fill(bx);
    }


     // fill histograms
     //____________________________________________
    if (bx_mask_col[bx-1]){

      for (const auto& muon: l1muons_){
        m_1dhist_["muonPt_c"]->Fill(muon.pt());
        m_1dhist_["muonPhi_c"]->Fill(muon.phi());
      }
    }

    if (bx_mask_noncol[bx-1]){

      for (const auto& muon: l1muons_){
        m_1dhist_["muonPt_nc"]->Fill(muon.pt());
        m_1dhist_["muonPhi_nc"]->Fill(muon.phi());
      }
    }

    if (bx_mask_ag[bx-1]){

      for (const auto& muon: l1muons_){
        m_1dhist_["muonPt_ag"]->Fill(muon.pt());
        m_1dhist_["muonPhi_ag"]->Fill(muon.phi());
      }
    }

    //____________________________________________
    //____________________________________________
    if (bx_mask_col[bx-1]){

      for (const auto& jet: l1jets_)
      {

        m_1dhist_["jetEt_c"]->Fill(jet.et());
        m_1dhist_["jetPhi_c"]->Fill(jet.phi());
      }
    }

    if (bx_mask_noncol[bx-1]){

      for (const auto& jet: l1jets_)
      {

        m_1dhist_["jetEt_nc"]->Fill(jet.et());
        m_1dhist_["jetPhi_nc"]->Fill(jet.phi());

      }

    }

    if (bx_mask_ag[bx-1]){
      for (const auto& jet: l1jets_)
      {

        m_1dhist_["jetEt_ag"]->Fill(jet.et());
        m_1dhist_["jetPhi_ag"]->Fill(jet.phi());

      }

    }


    //____________________________________________
    //____________________________________________

    if (bx_mask_col[bx-1]){
      for (const auto& egamma: l1egs_){

         m_1dhist_["egEt_c"]->Fill(egamma.et());
         m_1dhist_["egPhi_c"]->Fill(egamma.phi());

      }

    }

    if (bx_mask_noncol[bx-1]){
      for (const auto& egamma: l1egs_){

          m_1dhist_["egEt_nc"]->Fill(egamma.et());
          m_1dhist_["egPhi_nc"]->Fill(egamma.phi());

        }


    }

    if (bx_mask_ag[bx-1]){
      for (const auto& egamma: l1egs_){

          m_1dhist_["egEt_ag"]->Fill(egamma.et());
          m_1dhist_["egPhi_ag"]->Fill(egamma.phi());

        }

    }


    //____________________________________________
    for (const auto& jet: l1jets_){
      m_1dhist_["Jets"]->Fill(bx);
    }

    for (const auto& egamma: l1egs_){
      m_1dhist_["eGammas"]->Fill(bx);
    }
    // number of jets in bx

    for (const auto& tau: l1taus_){
      m_1dhist_["Taus"]->Fill(bx);
    }

    // muons
    if (muons_b.find(lumisection_) == muons_b.end()) {
      muons_b[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (size_t i = 0; i < l1muons_.size(); ++i) {
      muons_b[lumisection_][bx] += 1;
    }

    // jets
    if (jets_b.find(lumisection_) == jets_b.end()) {
      jets_b[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (const auto& jet: l1jets_){
      jets_b[lumisection_][bx] += jet.pt();
    }

    // eGammas
    if (eGammas_b.find(lumisection_) == eGammas_b.end()) {
      eGammas_b[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (size_t i = 0; i < l1egs_.size(); ++i) {
      eGammas_b[lumisection_][bx] += 1;
    }

    // taus
    if (taus_b.find(lumisection_) == taus_b.end()) {
      taus_b[lumisection_] = std::vector<float>(3564, 0.0f);
    }
    for (size_t i = 0; i < l1taus_.size(); ++i) {
      taus_b[lumisection_][bx] += 1;
    }

    // esum
    if (bxSums.size()>0) {

      l1t::EtSum l1sum;

      if (esum_b.find(lumisection_) == esum_b.end()) {
        esum_b[lumisection_] = std::vector<float>(3564, 0.0f);
      }

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kTotalEt);
      float pt_value = static_cast<float>(l1sum.pt());
      esum_b[lumisection_][bx] += pt_value;

      if (missing_b.find(lumisection_) == missing_b.end()) {
        missing_b[lumisection_] = std::vector<float>(3564, 0.0f);
      }

      l1sum = getL1TEtSum(bxSums[0], l1t::EtSum::EtSumType::kMissingEt);
      float et_value = static_cast<float>(l1sum.pt());
      missing_b[lumisection_][bx] += et_value;
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
  for (const auto& [key, _] : muons_b) {
      if (key < minLS) minLS = key;
      if (key > maxLS) maxLS = key;
  }

  const int nLumiBins = maxLS - minLS + 1;

  m_2dhist_["MuonBxOcc2D"] = histoSubDir.make<TH2D>( "MuonBxOcc2D", "Muon per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : muons_b) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["MuonBxOcc2D"]->Fill(key, bx, values[bx]);
    }
  }

  muons_b.clear();


  // jets
  m_2dhist_["JetBxOcc2D"] = histoSubDir.make<TH2D>( "JetBxOcc2D", "Jet per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : jets_b) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["JetBxOcc2D"]->Fill(key, bx, values[bx]);
    }
  }

  jets_b.clear();


  // eGammas
  m_2dhist_["eGammaBxOcc2D"] = histoSubDir.make<TH2D>( "eGammaBxOcc2D", "eGamma per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : eGammas_b) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["eGammaBxOcc2D"]->Fill(key, bx, values[bx]);
    }
  }

  eGammas_b.clear();


  // taus
  m_2dhist_["tauBxOcc2D"] = histoSubDir.make<TH2D>( "tauBxOcc2D", "tau per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : taus_b) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["tauBxOcc2D"]->Fill(key, bx, values[bx]);
    }
  }

  taus_b.clear();

  // Missing energy
  m_2dhist_["missing2D"] = histoSubDir.make<TH2D>( "missing2D", "missing energy per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : missing_b) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["missing2D"]->Fill(key, bx, values[bx]);
    }
  }

  missing_b.clear();

  // Energy sum
  m_2dhist_["energySum2D"] = histoSubDir.make<TH2D>( "energySum2D", "energy per bcid vs Lumisection"
    , nLumiBins, minLS - 0.5, maxLS + 0.5
    , nBX, -0.5, nBX - 0.5
    );

  for (const auto& [key, values] : esum_b) {
    for (int bx = 0; bx < nBX; ++bx) {
      m_2dhist_["energySum2D"]->Fill(key, bx, values[bx]);
    }
  }

  esum_b.clear();

}

DEFINE_FWK_MODULE(DemoAnalyzer);
