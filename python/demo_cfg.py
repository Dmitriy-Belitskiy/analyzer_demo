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

options.parseArguments()

process = cms.Process( "SCANALYZER" )

process.maxEvents = cms.untracked.PSet(
  input = cms.untracked.int32(options.numOrbits)
)

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.threshold = "WARNING"

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

process.scAnalyzer = cms.EDAnalyzer("DemoAnalyzer",
  muonsTag      = cms.InputTag("l1ScGmtUnpacker", "Muon", "SCHLP"),
  jetsTag       = cms.InputTag("l1ScCaloUnpacker", "Jet", "SCHLP"),
  eGammasTag    = cms.InputTag("l1ScCaloUnpacker", "EGamma", "SCHLP"),
  tausTag       = cms.InputTag("l1ScCaloUnpacker", "Tau", "SCHLP"),
  bxSumsTag     = cms.InputTag("l1ScCaloUnpacker", "EtSum", "SCHLP"),
)

process.p = cms.Path(
  process.scAnalyzer
)
