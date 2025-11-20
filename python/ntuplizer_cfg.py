import FWCore.ParameterSet.Config as cms
import FWCore.ParameterSet.VarParsing as VarParsing
import sys

options = VarParsing.VarParsing ('analysis')

options.register ("numOrbits",
                  -1,
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.int,
                  "Number of orbits/events to process")

options.register ("inFile",
                  "file:",
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "Path to the input file")

options.register ("outFile",
                  "file:/tmp/out.root",
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "Path of the output file")

options.register ("onlineSelection",
                  "",
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "Online Selectiom product tag")

options.parseArguments()

process = cms.Process( "SCNTUPLIZER" )

process.maxEvents = cms.untracked.PSet(
  input = cms.untracked.int32(options.numOrbits)
)

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.threshold = "WARNING"
process.MessageLogger.cerr.FwkReport.reportEvery = 10

process.Timing = cms.Service("Timing",
  summaryOnly = cms.untracked.bool(True),
  useJobReport = cms.untracked.bool(True)
)

process.TFileService = cms.Service("TFileService",
    fileName = cms.string(options.outFile)
)

process.source = cms.Source("PoolSource",
  fileNames = cms.untracked.vstring(options.inFile)
)

# passed an online selection tag
if options.onlineSelection != "":
    process.scNtuplizer = cms.EDAnalyzer("ScNtuplizer",
        muonsTag      = cms.InputTag("FinalBxSelector", "Muon"),
        jetsTag       = cms.InputTag("FinalBxSelector", "Jet"),
        eGammasTag    = cms.InputTag("FinalBxSelector", "EGamma"),
        bxSumsTag     = cms.InputTag("FinalBxSelector", "EtSum"),
        onlineSelection = cms.untracked.bool(True),
        selectedBxTag = cms.InputTag(options.onlineSelection, "SelBx"),
    )
else:
    # process all filled bx, ZB stream
    process.scNtuplizer = cms.EDAnalyzer("ScNtuplizer",
        muonsTag      = cms.InputTag("l1ScGmtUnpacker", "Muon"),
        jetsTag       = cms.InputTag("l1ScCaloUnpacker",  "Jet"),
        eGammasTag    = cms.InputTag("l1ScCaloUnpacker",  "EGamma"),
        bxSumsTag     = cms.InputTag("l1ScCaloUnpacker",  "EtSum"),
        onlineSelection = cms.untracked.bool(False)
    )

process.p = cms.Path(
  process.scNtuplizer
)
