// GARLI version 0.96b4 source code
// Copyright  2005-2006 by Derrick J. Zwickl
// All rights reserved.
//
// This code may be used and modified for non-commercial purposes
// but redistribution in any form requires written permission.
// Please contact:
//
//  Derrick Zwickl
//	National Evolutionary Synthesis Center
//	2024 W. Main Street, Suite A200
//	Durham, NC 27705
//  email: zwickl@nescent.org
//


#include <string.h>
#include <cassert>
#include <iostream>

using namespace std;

#include "defs.h"
#include "configoptions.h"
#include "configreader.h"
#include "errorexception.h"

/////////////////////////////////////////////////////////////////////////
// GamlConfig::General methods //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

GeneralGamlConfig::GeneralGamlConfig(){
	//Default values for everything

	//output related
	ofprefix = "ofprefix";
	logevery = 10;
	saveevery = 100;
	outputTreelog = false;
	outputMostlyUselessFiles = false;
	outputPhylipTree = false;
	writeCurrentBestTree = false;

	//starting the run
	randseed = -1;
	streefname = "random";
	refineStart = true;

	//general run details
	datafname = "datafname";
	constraintfile = "\0";
	availableMemory = -1; 
	megsClaMemory = 512;
	restart = false;
	checkpoint = false;
	significantTopoChange = (FLOAT_TYPE)0.01;
	searchReps = 1;
	//this isn't for general consumption, but lets me easily enable hacked in features
	runmode = 0;
	
	attachmentsPerTaxon = 50;

	//finishing the run
	enforceTermConditions = true;
	lastTopoImproveThresh = 10000;
	improveOverStoredIntervalsThresh = (FLOAT_TYPE)0.05;
	stopgen = UINT_MAX;
	stoptime = UINT_MAX;

	//default model is GTR+I+G
	stateFrequencies = "estimate";
	rateMatrix = "6rate";
	proportionInvariant = "estimate";
	rateHetModel = "gamma";
	numRateCats = 4;
	datatype = "dna";
	geneticCode = "standard";

	//general population stuff
	nindivs = 4;
	holdover = 1;
	selectionIntensity = 0.5;
	holdoverPenalty = 0.0;

	startOptPrec = 0.5;
	minOptPrec = (FLOAT_TYPE)0.01;
	numPrecReductions = 20;
	precReductionFactor = (FLOAT_TYPE)0.9;
	treeRejectionThreshold = 50.0;

	//parameters affecting proportion of mutations
	topoWeight = 1.0;
		randNNIweight = (FLOAT_TYPE)0.1;
		randSPRweight = (FLOAT_TYPE)0.3;
		limSPRweight = (FLOAT_TYPE)0.6;
	modWeight = (FLOAT_TYPE)0.05;
	brlenWeight = (FLOAT_TYPE)0.2;

	intervalLength = 100;
	intervalsToStore = 5;

	//parameters affecting other details of mutations	
	meanBrlenMuts = 5.0;
	gammaShapeBrlen = 1000;
	gammaShapeModel = 1000;
	limSPRrange = 6;
	uniqueSwapBias = (FLOAT_TYPE)0.1;
	distanceSwapBias = 1.0;
	
	//optional analyses
	inferInternalStateProbs = false;
	bootstrapReps = 0;

	sendInterval = 60.0;

	//these macros are all defined in the defs.h file
	//but could be overriden in the config
	minBrlen = DEF_MIN_BRLEN;
	maxBrlen = DEF_MAX_BRLEN;
	startingBrlen = DEF_STARTING_BRLEN;
	}

int GeneralGamlConfig::Read(const char* fname, bool isMaster /*=false*/)	{

	ConfigReader cr;	
	if (cr.Load(fname) != 0)	{
		printf("ERROR: GamlConfig::General::Read(%s) failed.\n", fname);
		return -1;
	}
	
	int errors = 0;
	errors += cr.SetSection("general");
	if(errors < 0) throw ErrorException("Didn't find [general] section in config file\n     (this section heading is required)");
	errors += cr.GetUnsignedOption("logevery", logevery);
	errors += cr.GetUnsignedOption("saveevery", saveevery);
	int found=cr.GetPositiveDoubleOption("megsclamemory", megsClaMemory, true);
	found += cr.GetPositiveDoubleOption("availablememory", availableMemory, true);
	if(found == -2) throw ErrorException("Either \"megsclamemory\" or \"availablememory\" must be specified in conf!");
	
	errors += cr.GetStringOption("datafname", datafname);
	errors += cr.GetStringOption("ofprefix", ofprefix);
	errors += cr.GetStringOption("streefname", streefname);
	cr.GetStringOption("constraintfile", constraintfile, true);
	errors += cr.GetIntOption("randseed", randseed);
	errors += cr.GetBoolOption("refinestart", refineStart);
	errors += cr.GetBoolOption("outputeachbettertopology", outputTreelog);
	errors += cr.GetBoolOption("enforcetermconditions", enforceTermConditions);
	errors += cr.GetUnsignedOption("genthreshfortopoterm", lastTopoImproveThresh);
	errors += cr.GetPositiveDoubleOption("scorethreshforterm", improveOverStoredIntervalsThresh);
	cr.GetPositiveDoubleOption("significanttopochange", significantTopoChange, true);
	cr.GetUnsignedOption("attachmentspertaxon", attachmentsPerTaxon, true);

	cr.GetBoolOption("outputmostlyuselessfiles", outputMostlyUselessFiles, true);
	cr.GetBoolOption("outputphyliptree", outputPhylipTree, true);
	cr.GetBoolOption("writecurrentbesttree", writeCurrentBestTree, true);

	cr.GetBoolOption("restart", restart, true);
	cr.GetBoolOption("writecheckpoints", checkpoint, true);

	cr.GetUnsignedOption("searchreps", searchReps, true);
	cr.GetUnsignedOption("runmode", runmode, true);

	//cr.GetBoolOption("useflexrates", useflexrates, true);
	//cr.GetBoolOption("dontinferproportioninvariant", dontInferProportionInvariant, true);
	cr.GetStringOption("ratehetmodel", rateHetModel, true);
	cr.GetUnsignedOption("numratecats", numRateCats, true);	
	cr.GetStringOption("statefrequencies", stateFrequencies, true);
	cr.GetStringOption("ratematrix", rateMatrix, true);
	cr.GetStringOption("invariantsites", proportionInvariant, true);
	cr.GetStringOption("datatype", datatype, true);
	cr.GetStringOption("geneticcode", geneticCode, true);

	cr.GetStringOption("outgroup", outgroupString, true);

	if(isMaster){
		errors += cr.SetSection("master");
		if(errors < 0) throw ErrorException("Didn't find [master] section in config file\n     (this section heading is required)");
		}
	else{
		errors += cr.SetSection("remote");
		if(errors < 0) throw ErrorException("Didn't find [remote] section in config file\n     (this section heading is required)");
		}
	
	errors += cr.GetUnsignedOption("nindivs", nindivs);
	errors += cr.GetUnsignedOption("holdover", holdover);
	errors += cr.GetPositiveDoubleOption("selectionintensity", selectionIntensity);
	errors += cr.GetDoubleOption("holdoverpenalty", holdoverPenalty);
	errors += cr.GetUnsignedOption("stopgen", stopgen);
	errors += cr.GetUnsignedOption("stoptime", stoptime);
	errors += cr.GetPositiveDoubleOption("startoptprec", startOptPrec);
	errors += cr.GetPositiveDoubleOption("minoptprec", minOptPrec);
	//changing this to specify either the number of reductions in the precision or the 
	//multiplier as before.  Prefer the number, since it should be easier to specify.  
	//
	found=0;
	found=cr.GetIntOption("numberofprecreductions", numPrecReductions, true);
	found += cr.GetPositiveDoubleOption("precreductionfactor", precReductionFactor, true);
	if(found == -2) throw ErrorException("Error: either \"numberofprecreductions\" (preferably) or \"precreductionfactor\" must be specified in conf!");
	
	errors += cr.GetPositiveDoubleOption("topoweight", topoWeight);
	errors += cr.GetPositiveDoubleOption("modweight", modWeight);	
	errors += cr.GetPositiveDoubleOption("brlenweight", brlenWeight);	
	errors += cr.GetPositiveDoubleOption("randnniweight", randNNIweight);
	errors += cr.GetPositiveDoubleOption("randsprweight", randSPRweight);	
	errors += cr.GetPositiveDoubleOption("limsprweight", limSPRweight);
	
	cr.GetPositiveDoubleOption("uniqueswapbias", uniqueSwapBias, true);
	cr.GetPositiveDoubleOption("distanceswapbias", distanceSwapBias, true);

	cr.GetDoubleOption("treerejectionthreshold", treeRejectionThreshold, true);

	cr.GetUnsignedOption("bootstrapreps", bootstrapReps, true);
#ifdef MPI_VERSION
	if(bootstrapReps != 0) throw ErrorException("Sorry, Bootstrap not yet implemented in parallel GARLI!");
#endif

	cr.GetBoolOption("inferinternalstateprobs", inferInternalStateProbs, true);

#ifdef MPI_VERSION
	if(isMaster==false) errors += cr.GetDoubleOption("sendinterval", sendInterval);
#endif
	

#ifdef GANESH
	errors += cr.GetDoubleOption("randpecrweight", randPECRweight);
#endif
//	errors += cr.GetUnsignedOption("gammashapebrlen", gammaShapeBrlen);	
//	errors += cr.GetUnsignedOption("gammashapemodel", gammaShapeModel);
	errors += cr.GetPositiveDoubleOption("gammashapebrlen", gammaShapeBrlen);	
	errors += cr.GetPositiveDoubleOption("gammashapemodel", gammaShapeModel);


	errors += cr.GetUnsignedOption("limsprrange", limSPRrange);
	errors += cr.GetUnsignedOption("intervallength", intervalLength);
	errors += cr.GetUnsignedOption("intervalstostore", intervalsToStore);
	errors += cr.GetPositiveDoubleOption("meanbrlenmuts", meanBrlenMuts);

	cr.GetPositiveDoubleOption("minbrlen", minBrlen, true);
	cr.GetPositiveDoubleOption("maxbrlen", maxBrlen, true);
	cr.GetPositiveDoubleOption("startingbrlen", startingBrlen, true);

#ifdef INCLUDE_PERTURBATION
	errors += cr.SetSection("perturbation");

	errors += cr.GetIntOption("perttype", pertType);
	errors += cr.GetDoubleOption("pertthresh", pertThresh);
	errors += cr.GetIntOption("minpertinterval", minPertInterval);
	errors += cr.GetIntOption("maxpertsnoimprove", maxPertsNoImprove);
	errors += cr.GetBoolOption("restartafterabandon", restartAfterAbandon);
	errors += cr.GetIntOption("gensbeforerestart", gensBeforeRestart);
	
	errors += cr.GetDoubleOption("ratchetproportion", ratchetProportion);
	errors += cr.GetDoubleOption("ratchetoffthresh", ratchetOffThresh);
	errors += cr.GetIntOption("ratchetmaxgen", ratchetMaxGen);
	
	errors += cr.GetIntOption("nnitargetaccepts", nniTargetAccepts);
	errors += cr.GetIntOption("nnimaxattempts", nniMaxAttempts);
	
	errors += cr.GetIntOption("numsprcycles", numSprCycles);
	errors += cr.GetIntOption("sprpertrange", sprPertRange);
#endif
	return errors;
}

int GeneralGamlConfig::Serialize(char** buf_, int* size_) const	{
	int& size = *size_;
	char*& buf = *buf_;
	
	// calculate the size first
	size = 0;
	size += sizeof(logevery);
	size += sizeof(saveevery);
	size += sizeof(megsClaMemory);
	
	size += (int)datafname.length() + 1;
	size += (int)ofprefix.length() + 1;
	size += (int)streefname.length() + 1;
	
	// allocate the buffer
	buf = new char[size];
	
	// populate the buffer
	char* p = buf;
	
	for(int i=0;i<size;i++){
		p[i]=0;
		}
	
	memcpy(p, &logevery, sizeof(logevery));
	p += sizeof(logevery);
	
	memcpy(p, &saveevery, sizeof(saveevery));
	p += sizeof(saveevery);
	
	memcpy(p, &megsClaMemory, sizeof(megsClaMemory));
	p += sizeof(megsClaMemory);
	
	memcpy(p, datafname.c_str(), datafname.length()+1);
	p += datafname.length()+1;
	
	memcpy(p, ofprefix.c_str(), ofprefix.length()+1);
	p += ofprefix.length()+1;

	memcpy(p, streefname.c_str(), streefname.length()+1);
	p += streefname.length()+1;
	
	// sanity checks
	assert(p-buf == size);
	
	return size;
}

int GeneralGamlConfig::Deserialize(char* buf, int size)	{

	char* p = buf;
	
	memcpy(&logevery, p, sizeof(logevery));
	p += sizeof(logevery);
	
	memcpy(&saveevery, p, sizeof(saveevery));
	p += sizeof(saveevery);
	
	memcpy(&megsClaMemory, p, sizeof(megsClaMemory));
	p += sizeof(megsClaMemory);	

	datafname = p;
	p += strlen(p)+1;
	
	ofprefix = p;
	p += strlen(p)+1;
	
	streefname = p;
	p += strlen(p)+1;
	
	// sanity checks
	assert(buf+size == p);
	
	return 0;
}

bool GeneralGamlConfig::operator==(const GeneralGamlConfig& rhs) const	{
	if (	logevery != rhs.logevery			||
			saveevery != rhs.saveevery			||
			datafname != rhs.datafname			||
			ofprefix != rhs.ofprefix	)
		return false;
	return true;
}

MasterGamlConfig::MasterGamlConfig() : GeneralGamlConfig() {
	
	
	}

int MasterGamlConfig::Read(const char* fname, bool isMaster){
	ConfigReader cr;	
	if (cr.Load(fname) != 0)	{
		printf("ERROR: GamlConfig::General::Read(%s) failed.\n", fname);
		return -1;
	}

	int errors = 0;

	errors += GeneralGamlConfig::Read(fname, true);

#ifdef MPI_VERSION
	
	errors += cr.SetSection("master");

	errors += cr.GetDoubleOption("startupdatethresh", startUpdateThresh);
	errors += cr.GetDoubleOption("minupdatethresh", minUpdateThresh);
//	errors += cr.GetDoubleOption("updatereductionfactor", updateReductionFactor);
				
//	errors += cr.GetIntOption("parallelinterval", subtreeInterval);
#ifdef SUBTREE_VERSION
	errors += cr.GetDoubleOption("subtreestartthresh", subtreeStartThresh);
	errors += cr.GetIntOption("minsubtreesize", minSubtreeSize);
	errors += cr.GetIntOption("targetsubtreesize", targetSubtreeSize);
	errors += cr.GetDoubleOption("orphanfactor", orphanFactor);
#else 
	subtreeStartThresh = 0.0;
	minSubtreeSize = -1;
	targetSubtreeSize = -1;
	orphanFactor = -1.0;
#endif

	errors += cr.GetIntOption("maxrecomindivs", maxRecomIndivs);
#endif
	return errors;
	}

/*
int GamlConfig::Master::Read(const char* fname)	{

	ConfigReader cr;	
	if (cr.Load(fname) != 0)	{
		printf("ERROR: GamlConfig::Master::Read(%s) failed.\n", fname);
		return -1;
	}
	
	int errors = 0;
	errors += cr.SetSection("master");
//	errors += cr.GetIntOption("crunchgens", crunchgens);
	errors += cr.GetIntOption("gammashape", gammashape);
	errors += cr.GetIntOption("holdover", holdover);
	errors += cr.GetIntRangeOption("nindivs", min_nindivs, max_nindivs);
//	errors += cr.GetIntOption("memory", memory);
//	errors += cr.GetIntOption("interval", interval);
//	errors += cr.GetIntOption("recvcount", recvcount);
	errors += cr.GetIntOption("stopgen", stopgen);
	errors += cr.GetDoubleRangeOption("meanbrlenmuts", min_brlen_muts, max_brlen_muts);
	
	errors += cr.GetDoubleOption("initialupdatethresh", initialUpdateThresh);
	errors += cr.GetDoubleOption("nonsubtreeupdatethresh", nonsubtreeUpdateThresh);
				
	errors += cr.GetIntOption("subtreeinterval", subtreeInterval);
	errors += cr.GetDoubleOption("subtreestartthresh", subtreeStartThresh);
	errors += cr.GetDoubleOption("subtreerecalcthresh", subtreeRecalcThresh);
	errors += cr.GetDoubleOption("subtreeupdatethresh", subtreeUpdateThresh);
	
	errors += cr.GetIntOption("perttype", pertType);
	errors += cr.GetDoubleOption("pertthresh", pertThresh);
	errors += cr.GetDoubleOption("pertamount", pertAmount);
	
	errors += cr.GetDoubleOption("selectionintensity", selectionIntensity);

	errors += cr.GetDoubleOption("startoptprec", startOptPrec);
	errors += cr.GetDoubleOption("minoptprec", minOptPrec);
	errors += cr.GetDoubleOption("topoweight", topoWeight);
	errors += cr.GetDoubleOption("modweight", modWeight);	
	errors += cr.GetDoubleOption("brlenweight", brlenWeight);	
	
	errors += cr.GetDoubleOption("randnniweight", randNNIweight);
	errors += cr.GetDoubleOption("randsprweight", randSPRweight);	
	errors += cr.GetDoubleOption("limsprweight", limSPRweight);
#ifdef GANESH
	errors += cr.GetDoubleOption("randpecrweight", randPECRweight);
#endif
	
	errors += cr.GetIntOption("limsprrange", limSPRrange);
	errors += cr.GetIntOption("intervallength", intervalLength);
	errors += cr.GetIntOption("intervalstostore", intervalsToStore);

	
		
	return -errors;
}

int GamlConfig::Master::Serialize(char** buf_, int* size_) const	{
	int& size = *size_;
	char*& buf = *buf_;
	
	// calculate the size first
	size = sizeof(*this);
	
	// allocate the buffer
	buf = new char[size];
	
	// populate the buffer
	char* p = buf;
	
	memcpy(p, this, sizeof(*this));
	p += sizeof(*this);
	
	// sanity checks
	assert(p-buf == size);
	
	return size;
}

int GamlConfig::Master::Deserialize(char* buf, int size)	{

	char* p = buf;
	
	memcpy(this, p, sizeof(*this));
	p += sizeof(*this);
	
	// sanity checks
	assert(buf+size == p);
	
	return 0;
}

bool GamlConfig::Master::operator==(const GamlConfig::Master& rhs) const	{
	if (memcmp(this, &rhs, sizeof(*this)) == 0)
		return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////
// GamlConfig::Remote methods ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

GamlConfig::Remote::Remote()	{
	memset(this, 0, sizeof(*this));
}

int GamlConfig::Remote::Read(const char* fname)	{

	ConfigReader cr;	
	if (cr.Load(fname) != 0)	{
		printf("ERROR: GamlConfig::Remote::Read(%s) failed.\n", fname);
		return -1;
	}
	
	int errors = 0;
	errors += cr.SetSection("remote");
	errors += cr.GetIntOption("gammashape", gammashape);
	errors += cr.GetIntOption("holdover", holdover);
	errors += cr.GetIntRangeOption("nindivs", min_nindivs, max_nindivs);
	errors += cr.GetIntOption("interval", interval);
	errors += cr.GetIntOption("stopgen", stopgen);
	errors += cr.GetDoubleRangeOption("meanbrlenmuts", min_brlen_muts, max_brlen_muts);
	errors += cr.GetDoubleOption("selectionintensity", selectionIntensity);
	errors += cr.GetDoubleOption("startoptprec", startOptPrec);
	errors += cr.GetDoubleOption("minoptprec", minOptPrec);
	errors += cr.GetDoubleOption("topoweight", topoWeight);
	errors += cr.GetDoubleOption("modweight", modWeight);	
	errors += cr.GetDoubleOption("brlenweight", brlenWeight);	
	
	errors += cr.GetDoubleOption("randnniweight", randNNIweight);
	errors += cr.GetDoubleOption("randsprweight", randSPRweight);	
	errors += cr.GetDoubleOption("limsprweight", limSPRweight);
#ifdef GANESH
	errors += cr.GetDoubleOption("randpecrweight", randPECRweight);
#endif
	
	errors += cr.GetIntOption("limsprrange", limSPRrange);
	errors += cr.GetIntOption("intervallength", intervalLength);
	errors += cr.GetIntOption("intervalstostore", intervalsToStore);

	return -errors;
}

int GamlConfig::Remote::Serialize(char** buf_, int* size_) const	{
	int& size = *size_;
	char*& buf = *buf_;
	
	// calculate the size first
	size = sizeof(*this);
	
	// allocate the buffer
	buf = new char[size];
	
	// populate the buffer
	char* p = buf;
	
	memcpy(p, this, sizeof(*this));
	p += sizeof(*this);
	
	// sanity checks
	assert(p-buf == size);
	
	return size;
}

int GamlConfig::Remote::Deserialize(char* buf, int size)	{

	char* p = buf;
	
	memcpy(this, p, sizeof(*this));
	p += sizeof(*this);
	
	// sanity checks
	assert(buf+size == p);
	
	return 0;
}

bool GamlConfig::Remote::operator==(const GamlConfig::Remote& rhs) const	{
	if (memcmp(this, &rhs, sizeof(*this)) == 0)
		return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////
// GamlConfig methods ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

GamlConfig::GamlConfig()	{
}

int GamlConfig::Read(const char* fname)	{
	int gerrors, merrors, rerrors;
	
	gerrors = -gc.Read(fname);
	if (gerrors > 0)	{
		printf("ERROR: GamlConfig::Read(): reading [general] produced %d errors.\n", gerrors);
	}
	
	merrors = -mc.Read(fname);
	if (merrors > 0)	{
		printf("ERROR: GamlConfig::Read(): reading [master] produced %d errors.\n", merrors);
	}
	
	rerrors = -rc.Read(fname);
	if (rerrors > 0)	{
		printf("ERROR: GamlConfig::Read(): reading [remote] produced %d errors.\n", rerrors);
	}
	
	if (gerrors || merrors || rerrors)
		return -1;
		
	return 0;
}

int GamlConfig::Serialize(char** buf_, int* size_) const{
	//there's no need to serialize and send the master conf info	
	
	int& size = *size_;
	char*& buf = *buf_;
	
	int gsize, msize, rsize;
	char *gbuf, *mbuf, *rbuf;
	
	gc.Serialize(&gbuf, &gsize);
//	mc.Serialize(&mbuf, &msize);
	rc.Serialize(&rbuf, &rsize);
	
	size = gsize + rsize + sizeof(int)*2;
//	size = gsize + msize + rsize + sizeof(int)*3;
	
	char* p = buf = new char[size];
	
	// put in the sizes
	
	memcpy(p, &gsize, sizeof(gsize));
	p += sizeof(gsize);
	
//	memcpy(p, &msize, sizeof(msize));
//	p += sizeof(msize);
	
	memcpy(p, &rsize, sizeof(rsize));
	p += sizeof(rsize);
	
	// put in the data

	memcpy(p, gbuf, gsize);
	p += gsize;


//	memcpy(p, mbuf, msize);
//	p += msize;
	
	memcpy(p, rbuf, rsize);
	p += rsize;
	
	delete [] gbuf;
//	delete [] mbuf;
	delete [] rbuf;
	
	return size;
}

int GamlConfig::Deserialize(char* buf, int size)	{
	int gsize, msize, rsize;
	
	char* p = buf;
	
	memcpy(&gsize, p, sizeof(gsize));
	p += sizeof(gsize);
*/
/*	
	memcpy(&msize, p, sizeof(msize));
	p += sizeof(msize);
*/	
/*	memcpy(&rsize, p, sizeof(rsize));
	p += sizeof(rsize);
	
	gc.Deserialize(p, gsize);
	p += gsize;
	rc.Deserialize(p, rsize);
	p += rsize;
	
	// sanity checks
	assert(p-buf == size);
	
	return 0;
}

bool GamlConfig::operator==(const GamlConfig& rhs) const	{
	if (	gc == rhs.gc	&&
			mc == rhs.mc	&&
			rc == rhs.rc
		)
		return true;
	return false;
}
*/

