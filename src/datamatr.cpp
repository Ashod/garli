// GARLI version 0.951 source code
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
//	NOTE: Portions of this source adapted from GAML source, written by Paul O. Lewis

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

using namespace std;

#include "defs.h"
#include "datamatr.h"
#include "rng.h"
#include "stricl.h"
#include "errorexception.h"
#include "outputman.h"

#define MAX_TAXON_LABEL		80

extern rng rnd;
extern OutputManager outman;

DataMatrix::~DataMatrix()
{
	if( count ) MEM_DELETE_ARRAY(count); // count is of length nChar
	if( numStates ) MEM_DELETE_ARRAY(numStates); // numStates is of length nChar
	if( stateDistr ) MEM_DELETE_ARRAY(stateDistr); // stateDistr is of length (maxNumStates+1)
	if( number ) MEM_DELETE_ARRAY(number); // number is of length nChar
	if( taxonLabel ) {
		int j;
		for( j = 0; j < nTax; j++ )
			MEM_DELETE_ARRAY( taxonLabel[j] ); // taxonLabel[j] is of length strlen(taxonLabel[j])+1
	       MEM_DELETE_ARRAY(taxonLabel); // taxonLabel is of length nTax
	}
	if( matrix ) {
		int j;
		for( j = 0; j < nTax; j++ )
			MEM_DELETE_ARRAY(matrix[j]); // matrix[j] is of length nChar
		MEM_DELETE_ARRAY(matrix); // matrix is of length nTax
	}
	if(constBases!=NULL) delete []constBases;
	if(origCounts!=NULL) delete []origCounts;
	memset(this, 0, sizeof(DataMatrix));
}

void DataMatrix::SetTaxonLabel(int i, const char* s)
{
	if( taxonLabel && (i < nTax) )
		ReplaceTaxonLabel(i, s);
}

void DataMatrix::ReplaceTaxonLabel( int i, const char* s )
{
	assert( taxonLabel );
	if( taxonLabel[i] ) {
		MEM_DELETE_ARRAY(taxonLabel[i]); // taxonLabel[i] is of length strlen(taxonLabel[i])+1
	}
	MEM_NEW_ARRAY(taxonLabel[i],char,(strlen(s)+1));
	strcpy(taxonLabel[i], s);
}

FLOAT_TYPE DataMatrix::prNumStates(int n) const
{
	assert( stateDistr );
	assert( stateDistrComputed );
	return ( n > maxNumStates ? (FLOAT_TYPE)0.0 : stateDistr[n] );
}

void DataMatrix::AllocPr( DblPtrPtr& pr )
{	int ns = maxNumStates;
	MEM_NEW_ARRAY(pr,FLOAT_TYPE*,ns);
    int i;
	
#ifndef CONTIG_PRMAT
	for( i = 0; i < ns; i++ )
		MEM_NEW_ARRAY(pr[i],FLOAT_TYPE,ns);
#else
	MEM_NEW_ARRAY(pr[0],FLOAT_TYPE,ns*ns);
	for(i=1;i<ns;i++){
		pr[i]=pr[i-1]+ns;
		}	
#endif
}

void DataMatrix::DeletePr( DblPtrPtr& pr )
{
	if( !pr ) return;
	int ns = maxNumStates;
#ifndef CONTIG_PRMAT
	for( int i = 0; i < ns; i++ ) {
	    MEM_DELETE_ARRAY(pr[i]); // pr[i] is of length nr
		pr[i] = 0;
	}
#else
	MEM_DELETE_ARRAY(pr[0]);


#endif
    MEM_DELETE_ARRAY(pr); // pr is of length ns
	pr = 0;
	
	
	
}

//
// PositionOf returns position (starting from 0) of taxon whose name
// matches the string s in the taxonLabel list
//
int DataMatrix::PositionOf( char* s ) const
{
	int i;

	for( i = 0; i < nTax; i++ ) {
		if( strcmp( taxonLabel[i], s ) == 0 ) break;
	}

	assert( i < nTax );

	return i;
}

//
// PatternType determines whether pattern k is constant, informative, or autoapomorphic
//
int DataMatrix::PatternType( int k , int *c, unsigned char *s) const
{
	if( k >= nChar )
		return 0;
	int i, j, retval;

	// create an array to store counts of taxa having particular states
//	int* c;
//      MEM_NEW_ARRAY(c,int,nTax);

	for( i = 0; i < nTax; i++ )
		c[i] = 1;

	// create an array to hold this pattern
//	unsigned char* s;
//       MEM_NEW_ARRAY(s,unsigned char,nTax);

	for( i = 0; i < nTax; i++ )
		s[i] = Matrix( i, k );

	// sort elements of s
	for( i = 0; i < nTax-1; i++ ) {
		for( j = i+1; j < nTax; j++ ) {
			if( s[i] > s[j] ) {
				unsigned char tmp = s[i];
				s[i] = s[j];
				s[j] = tmp;
			}
		}
	}
	
	// add counts of duplicate elements of s to first instance
	int nStates = 1; 	// treats ? as a new state
	bool ambig = false;	// will be true if any ? found
	bool allMissing = true;
	i = 0;
	//if( s[0] == MISSING_DATA ) missing = 1;
	if( s[0] & (s[0]-1) ) ambig = true;
	if(s[0]!=15)  allMissing=false;
	for( j = 1; j < nTax; j++ ) {
		if(s[j]!=15)  allMissing=false;
		if( s[j] == s[i] ) {
			c[i]++;
			c[j]--;
		}
		else {
			if( s[j] & (s[j]-1)) ambig=true;
			if((s[i] & s[j])==false){
				i = j;
				nStates++;
				}
		}
	}

	//DJZ 10/28/03 changing this to allow for invariant sites.  Sites which contain 
	//some missing data but are otherwise constant must be marked as such because they 
	//will be considered constant for the purposes of invariant sites calcs.
	//also marking sites that are all missing

//	if( nStates == 1 )
	if( nStates == 1 /*|| (nStates==2 && missing)*/)
		retval = PT_CONSTANT;
	else if( nStates == 2 && ( c[0] == 1 || c[0] == nTax-1 ) )
		retval = PT_AUTAPOMORPHIC | PT_VARIABLE;
	else if( nStates < nTax )
		retval = PT_INFORMATIVE | PT_VARIABLE;
	else
		retval = PT_VARIABLE;

//	MEM_DELETE_ARRAY(s); // s is of length nTax
//	MEM_DELETE_ARRAY(c); // c is of length nTax

//	numStates[k] = ( missing ? nStates-1 : nStates );
	if(allMissing) nStates=0;
	numStates[k] = nStates;
	return retval;
}

//
// Summarize tallies number of constant, informative, and autapomorphic characters
//
void DataMatrix::Summarize()
{
	int i, k;
	assert( nChar > 0 );

	nConstant = nInformative = nAutapomorphic = 0;
   int nTotal = 0;

   int max = maxNumStates;
   for( i = 0; i <= max; i++ )
	   stateDistr[i] = 0.0;

	//DJZ moved these out of PatternType to reduce the amount of allocation
	int *c = new int[nTax];
 	//MEM_NEW_ARRAY(c,int,nTax);
	unsigned char *s = new unsigned char[nTax];
//	MEM_NEW_ARRAY(s,unsigned char,nTax);
	
	for( k = 0; k < nChar; k++ ) {
		int ptFlags = PatternType(k, c, s);
      stateDistr[numStates[k]] += (FLOAT_TYPE)count[k];
      nTotal += count[k];

		if( ptFlags & PT_CONSTANT )
			nConstant += count[k];
		else if( ptFlags & PT_INFORMATIVE )
			nInformative += count[k];
		else if( ptFlags & PT_AUTAPOMORPHIC )
			nAutapomorphic += count[k];
	}

   for( k = 0; k <= max; k++ )
   	stateDistr[k] /= (FLOAT_TYPE)nTotal;
   stateDistrComputed = 1;
   
   delete []c;
   delete []s;
}

//
// NewMatrix deletes old matrix, taxonLabel, count, and number
// arrays and creates new ones
//
void DataMatrix::NewMatrix( int taxa, int sites )
{
	// delete taxon labels
	if( taxonLabel ) {
		int i;
		for( i = 0; i < nTax; i++ )
			MEM_DELETE_ARRAY(taxonLabel[i]); // taxonLabel[i] is of length strlen(taxonLabel[i])+1
		MEM_DELETE_ARRAY(taxonLabel); // taxonLabel is of length nTax
	}

	// create new array of taxon label pointers
	if( taxa > 0 ) {
		MEM_NEW_ARRAY(taxonLabel,char*,taxa);
		for( int i = 0; i < taxa; i++ )
			taxonLabel[i] = NULL;
	}

	// delete data matrix and count and number arrays
	if( matrix ) {
		int j;
	//	for( j = 0; j < nChar; j++ )
		for( j = 0; j < nTax; j++ )
			MEM_DELETE_ARRAY(matrix[j]); // matrix[j] has length nChar
		MEM_DELETE_ARRAY(matrix); // matrix has length nTax
	}

	if( count ) {
		MEM_DELETE_ARRAY(count); //count has length nChar
	}
	if( numStates ) {
		MEM_DELETE_ARRAY(numStates); // numStates has length nChar
	}
	if( stateDistr ) {
		MEM_DELETE_ARRAY(stateDistr); // stateDistr has length maxNumStates+1
	}
	if( number ) {
                MEM_DELETE_ARRAY(number); // number has length nChar
        }

	// create new data matrix, and new count and number arrays
	// all counts are initially 1, and characters are numbered
	// sequentially from 0 to nChar-1
	if( taxa > 0 && sites > 0 ) {
		MEM_NEW_ARRAY(matrix,unsigned char*,taxa);
		MEM_NEW_ARRAY(count,int,sites);
		MEM_NEW_ARRAY(numStates,int,sites);
		MEM_NEW_ARRAY(stateDistr,FLOAT_TYPE,(maxNumStates+1));
		MEM_NEW_ARRAY(number,int,sites);

		for( int j = 0; j < sites; j++ ) {
			count[j] = 1;
			numStates[j] = 1;
			number[j] = j;
		}
		for( int i = 0; i < taxa; i++ ) {
			matrix[i]=new unsigned char[sites];
			//MEM_NEW_ARRAY(matrix[i],unsigned char,sites);
			//memset( matrix[i], 0xff, taxa*sizeof(unsigned char) );
			memset( matrix[i], 0xff, sites*sizeof(unsigned char) );
		}
		int max = maxNumStates;
		for( int k = 0; k <= max; k++ )
			stateDistr[k] = 0.0;
	}

	// set dimension variables to new values
	nTax = taxa;
	gapsIncludedNChar = totalNChar = nChar = sites;
}

DataMatrix& DataMatrix::operator =(const DataMatrix& d)
{
	NewMatrix( d.NTax(), d.NChar() );

	int i, j;
	for( i = 0; i < nTax; i++ ) {
		SetTaxonLabel(i, d.TaxonLabel(i) );
	}

	for( j = 0; j < nChar; j++ ) {
		SetCount(j, d.Count(j) );
		number[j] = d.Number(j);
	}

	for( i = 0; i < nTax; i++ ) {
		for( j = 0; j < nChar; j++ )
			SetMatrix(i, j, d.Matrix(i, j) );
	}

	return *this;
}

//
// Pack simply deletes sites having a count of zero
//
void DataMatrix::Pack()
{
	int i, j, newNChar = 0;

	// determine dimensions of new matrix
	for( j = 0; j < nChar; j++ ) {
		if( count[j] )
			newNChar++;
	}

	// create new matrix and count and number arrays and fill
	unsigned char** newMatrix;
        MEM_NEW_ARRAY(newMatrix,unsigned char*,nTax);
	int* newCount;
        MEM_NEW_ARRAY(newCount,int,newNChar);
	int* newNumStates;
        MEM_NEW_ARRAY(newNumStates,int,newNChar);
//	int* newNumber;
  //      MEM_NEW_ARRAY(newNumber,int,newNChar);

	for( i = 0; i < nTax; i++ )
		 MEM_NEW_ARRAY(newMatrix[i],unsigned char,newNChar);


	i = 0;
	for( j = 0; j < nChar; j++ ) {
		if( count[j] ) {
			for( int k = 0; k < nTax; k++ )
				newMatrix[k][i] = matrix[k][j];
			newCount[i] = count[j];
			newNumStates[i] = numStates[j];
			//newNumber[i] = number[j];
			i++;
			}
		else{//as we remove columns, shift all the greater numbers over
			for(int c=0;c<gapsIncludedNChar;c++){
				if(number[c] >= i) number[c]--;
				}
			}
		}

	// copy distribution of the number of states
        int max = maxNumStates;
        FLOAT_TYPE* newStateDistr;
	MEM_NEW_ARRAY(newStateDistr,FLOAT_TYPE,(max+1));
        for( i = 0; i <= max; i++ )
   	        newStateDistr[i] = stateDistr[i];

	// delete old matrix and count and number arrays
	if( count ) MEM_DELETE_ARRAY(count); // count has length nChar
	if( numStates ) MEM_DELETE_ARRAY(numStates); // numStates has length nChar
	if( stateDistr ) MEM_DELETE_ARRAY(stateDistr); // stateDistr has length maxNumStates+1
//	if( number ) MEM_DELETE_ARRAY(number); // number has length nChar
	if( matrix ) {
		for( i = 0; i < nTax; i++ )
			MEM_DELETE_ARRAY(matrix[i]); // matrix[i] has length nChar
		MEM_DELETE_ARRAY(matrix); // matrix has length nTax
        }

	// set count, number and matrix to their new counterparts
	count = newCount;
	numStates = newNumStates;
	stateDistr = newStateDistr;
//	number = newNumber;
	matrix = newMatrix;
	nChar = newNChar;
	
}


void DataMatrix::DetermineConstantSites(){
	//DJZ 10/28/03 note where all of the constant sites are, and what they are
	//this is kind of ugly, but will never be rate limiting
	lastConstant=-1;	
	while(numStates[lastConstant+1]==1) lastConstant++;

	//now that I'm allowing partial ambiguity, the determination of constants needs to change
	//can't just depend on the numStates field
	
	constBases=new int[lastConstant+1];
	int t;
	for(int i=0;i<lastConstant+1;i++){
		t=0;
		char c=15;
		while(t<nTax){
		char ch=Matrix(t, i);
			c = c & ch;
			t++;
			}
		assert(c!=0);
//		while(Matrix(t, i)==MISSING_DATA) t++;
//		constBases[i]=Matrix(t, i);
		constBases[i]=c;
		}
/*	//now convert from the bitwise 1, 2, 4, 8 designation to 0, 1, 2, 3
	for(int i=0;i<lastConstant+1;i++){
		if(constBases[i]==1) constBases[i]=0;
		else if(constBases[i]==2) constBases[i]=1;
		else if(constBases[i]==4) constBases[i]=2;
		else if(constBases[i]==8) constBases[i]=3;
		}
*/
	origCounts=new int[nChar];
	}

//
//	SwapCharacters swaps matrix column i with column j
//
void DataMatrix::SwapCharacters( int i, int j )
{
	unsigned char tmp;
	for( int k = 0; k < nTax; k++ ) {
		tmp = Matrix( k, i );
		SetMatrix( k, i, Matrix( k, j ) );
		SetMatrix( k, j, tmp );
		
	}
	//DJZ also swap the nStates array
	int s=numStates[i];
	numStates[i]=numStates[j];
	numStates[j]=s;

	//DJZ 2/14/06 and the number array
	for(int c=0;c<gapsIncludedNChar;c++){
		if(number[c] == i) number[c]=j;
		else if(number[c] == j) number[c]=i;
		}
}

void DataMatrix::BeginNexusTreesBlock(ofstream &treeout){
	//this outputs everything up through the translate table
	treeout << "#NEXUS\n\nbegin trees;\ntranslate\n";
	for(int k=0;k<nTax;k++){
		treeout << "  " << (k+1);
		NxsString tnstr = TaxonLabel(k);
		tnstr.blanks_to_underscores();
		treeout << "  " << tnstr.c_str();
		if( k == nTax-1 )
			treeout << ";\n";
		else
			treeout << ",\n";
		}
	}

//
// ComparePatterns returns:
//	 0		complete identity
//	-1		if i less than j
//	 1		if i greater than j
//
int DataMatrix::ComparePatterns( const int i, const int j ) const
{		
	//DJZ 10/28/03 altering this to always put constant patterns at the start, which will
	//make implementing invariant sites much easier.  

	int cmp = 0;
		
	if(numStates[i]==1){
		if(numStates[j]==1){
			if(Matrix(0,i) < Matrix(0,j)) return -1;
			//else return 1;
			else{
				for( int k = 0; k < nTax; k++ ) {
					int same = ( Matrix( k, i ) == Matrix( k, j ) );
					if( !same )	{
						FLOAT_TYPE diff = ( (FLOAT_TYPE)Matrix( k, i ) - (FLOAT_TYPE)Matrix( k, j ) );
						cmp = ( diff < 0.0 ? -1 : 1 );
						break;
					}
				}
				return cmp;
				}
			}
		else return -1;
		}
	else if(numStates[j]==1){
		return 1;
		}
	
	
	for( int k = 0; k < nTax; k++ ) {
		int same = ( Matrix( k, i ) == Matrix( k, j ) );
		if( !same )	{
			FLOAT_TYPE diff = ( (FLOAT_TYPE)Matrix( k, i ) - (FLOAT_TYPE)Matrix( k, j ) );
			cmp = ( diff < 0.0 ? -1 : 1 );
			break;
		}
	}
	return cmp;
}

//
// Collapse merges like patterns
//
void DataMatrix::Collapse(){
	int i = 0, j = 1;

	Sort();

	while( i < nChar ) {
		while( j < nChar && ComparePatterns( i, j ) == 0 ) {
			// pattern j same as pattern i
			count[i] += count[j];
			count[j] = 0;
			j++;
			}
		i = j++;
		}
		
	//DJZ 10/28/03 get rid of all missing patterns	
	int q=nChar-1;
	while(numStates[q]==0){
		for(i=0;i<gapsIncludedNChar;i++) if(number[i]==q) number[i]=-1;
		count[q--]=0;
		//when all missing columns are deleted, remove them from the total number of characters
		totalNChar--;
		}
	
	Pack();
	}

//
//  BSort implements a simple bubblesort
//
void DataMatrix::BSort( int byCounts /* = 0 */ )
{
	int swap, k;
	for( int i = 0; i < nChar-1; i++ ) {
		for( int j = i+1; j < nChar; j++ ) {
			if( byCounts )
				swap = ( count[i] < count[j] ? 1 : 0 );
			else
				swap = ( ComparePatterns( i, j ) > 0 ? 1 : 0 );
			if( swap ) {
				SwapCharacters( i, j );

				k = count[i];
				count[i] = count[j];
				count[j] = k;

				k = numStates[i];
				numStates[i] = numStates[j];
				numStates[j] = k;

				k = number[i];
				number[i] = number[j];
				number[j] = k;
			}
		}
	}
}

void DataMatrix::DebugSaveQSortState( int top, int bottom, int ii, int jj, int xx, const char* title )
{
	ofstream qsf( "qsstate.txt", ios::out | ios::app );
	qsf << endl << title << endl;

	int i, j;
	for( j = 0; j < nChar; j++ )
	{
		qsf << setw(6) << j << "  ";
		for( i = 0; i < nTax; i++ )
			qsf << DatumToChar( Matrix( i, j ) );
		if( j == top )
			qsf << " <-- top   ";
		if( j == ii )
			qsf << " <-- i     ";
		if( j == bottom )
			qsf << " <-- bottom";
		if( j == jj )
			qsf << " <-- j     ";
		if( j == xx )
			qsf << " <-- x     ";
		qsf << endl;
	}

	qsf.close();
}

//
//  QSort implements the quicksort algorithm
//
void DataMatrix::QSort( int top, int bottom )
{
	int i = top;
	int j = bottom;
	int x = ( top + bottom ) / 2;
	//DebugSaveQSortState( top, bottom, i, j, x, "Entering QSort" );
	do {
		while( ComparePatterns( i, x ) < 0  &&  i < bottom ) i++ ;
		while( ComparePatterns( x, j ) < 0  &&  j > top    ) j-- ;

		if( i <= j ) {
			//DebugSaveQSortState( top, bottom, i, j, x, "Just about to swap i and j" );
			SwapCharacters( i, j );

			if( x == i )		// keep track of the reference pattern!
				x = j;
			else if( x == j )	
				x = i;
			i++;
			if(j) j--;
			//DebugSaveQSortState( top, bottom, i, j, x, "Just after swapping" );
		}

	} while( i <= j );

	if( top <  j      ) QSort( top, j      );
	if( i   <  bottom ) QSort(   i, bottom );
}

int DataMatrix::GetToken( istream& in, char* tokenbuf, int maxlen, bool acceptComments /*=true*/ )
{
	int ok = 1;

	int i;
	char ch = ' ';

	// skip leading whitespace
	while( in && ( isspace(ch) || ch == '[' ) ){
		in.get(ch);
		if(ch == '[' && acceptComments==false) return -1;
		}
	if( !in ) return 0;

	tokenbuf[0] = ch;
	tokenbuf[1] = '\0';
	tokenbuf[maxlen-1] = '\0';
		
	for( i = 1; i < maxlen-1; i++ ) {
		in.get(ch);
		if( isspace(ch) || ch == ']' )
			break;
		tokenbuf[i] = ch;
		tokenbuf[i+1] = '\0';
	}

	if( i >= maxlen-1 )
		ok = 0;

	return ok;
}

//
// Read reads in data from a file

int DataMatrix::Read( const char* infname, char* left_margin )
{
	char ch;
	bool isNexus=false;

//	ofstream debug("debug.log");
	ofstream debug;//killing this stream
	left_margin=new char[20];
	strcpy(left_margin, "  ");

	ifstream inf( infname );
	assert( inf );

	// get comments (note: comments only allowed at the beginning of the file)
	int end_of_comments = 0;
	while( !end_of_comments )
	{
		inf.get(ch);
		if( ch != '/' ) {
			inf.putback(ch);
			end_of_comments = 1;
		}
		else {
			// ch is a slash, ignore rest of this line
			char tmp[256];
			while( ch != '\n' ) {
				inf.get( tmp, 255, '\n' ); // will not extract end of line character
				inf.get(ch);   // try to get the end of line character
			}
		}
	}

	// get the dimensions of the data file
	int num_taxa=0, num_chars=0;
	char buf[200];
	inf >> buf;
	if((!(strcmp(buf, "#NEXUS"))) || (!(strcmp(buf, "#nexus"))) || (!(strcmp(buf, "#Nexus")))){
		isNexus=true;
		while(strcmp(buf, "dimensions") && strcmp(buf, "Dimensions") && strcmp(buf, "DIMENSIONS")) inf >> buf;
		int numread=0;
		do{
			inf >> buf;
			numread++;
			if(buf[1]=='t' || buf[1]=='T'){
				num_taxa=atoi(buf+5);
				}
			if(buf[1]=='c' || buf[1]=='C'){
				num_chars=atoi(buf+6);
				}
			}while(numread < 2);
		if(!(num_taxa > 0) ){
			throw ErrorException("Problem reading ntax on dimensions line of Nexus file!");
			}
		if(!(num_chars > 0) ){
			throw ErrorException("Problem reading nchar on dimensions line of Nexus file!");
			return -1;
			}
		do{
			inf >> buf;
			if(inf.eof()){
				throw ErrorException("Could not find data matrix!");
				return -1;
				}
			}while(strcmp(buf, "matrix") && strcmp(buf, "Matrix") && strcmp(buf, "MATRIX"));
		}		
	else{
		num_taxa=atoi(buf);
		inf >> num_chars;
		}

	assert(num_taxa<30000);	//if this trips ToplogyList will have to be modified (ints to longs
	NewMatrix( num_taxa, num_chars );

	// read in the data, including taxon names
	for( int i = 0; i < num_taxa; i++ ) {

		// get name for taxon i
		char taxon_name[ MAX_TAXON_LABEL ];
		int ok = GetToken( inf, taxon_name, MAX_TAXON_LABEL, false);
		if(ok == -1){
			throw ErrorException("\nERROR: Confused by comments (i.e. [...]) in datafile.\nPlease remove comments by exporting dataset from PAUP* or a similar program.\n");			
			}
		if( !ok ) {
			if( left_margin )
				debug << left_margin << "Error reading data: label for taxon " << (i+1) << " too long" << endl;
			Flush();
			return 0;
		}
		SetTaxonLabel( i, taxon_name );

		// get data for taxon i
		for( int j = 0; j < num_chars; j++ ) {
			inf >> ch;
			if(ch == '['){//if there is a comment here, which is how the "color" used to be represented
				while (ch != ']') inf >> ch;
				inf >> ch;
				}
			unsigned char datum;
			if( ch == '.' ) 
	    		datum = Matrix( 0, j );
		 		
	 		else 
				datum = CharToBitwiseRepresentation(ch);
				
			SetMatrix( i, j, datum );
			}
		}

	// read in the line containing the counts
	//DZ 11-25-02 This left_margin stuff seems to be totally screwed up, and results in the 
	//counts not being read.  Commenting out.
	if( inf ) {
		int i;

		for( i = 0; i < num_chars; i++ ) {
			int cnt;			

			inf >> cnt;
			if( !inf ) break;
			SetCount( i, cnt );
		}
		if( inf && i == num_chars ) {
			if( left_margin )
				debug << left_margin << "Counts read sucessfully" << endl;
			dense = 1;
		}
		else if( !inf && i > 0 ) {
			if( left_margin )
				debug << left_margin << "Error reading count number " << i << endl;
		}
		else {
			if( left_margin )
				debug << left_margin << "No counts found" << endl;
		}
		//DJZ 9-13-06
		//It is very important to properly set the totalNChar variable now
		//to be the sum of the counts, otherwise bootstrapping after reading
		//a .cond file will give wrong resampling!!!!!
		totalNChar=0;
		for(int i=0;i<num_chars;i++){
			totalNChar += count[i];
			}


	}

	// read in the line containing the number of states for each character
	if( inf ) {
		int i;
		for( i = 0; i < num_chars; i++ ) {
			int nstates;
			inf >> nstates;
			if( !inf ) break;
			SetNumStates( i, nstates );
			// njstart.numstates[i]=nstates;
		}
		if( inf && i == num_chars ) {
			if( left_margin )
				debug << left_margin << "Number of states for each character read sucessfully" << endl;
			dense = 1;
		}
		else if( !inf && i > 0 ) {
			if( left_margin )
				debug << left_margin << "Error reading number of states for character number " << i << endl;
		}
		else {
			if( left_margin )
				debug << left_margin << "Line containing number of states for each character not found" << endl;
		}
	}

	inf.close();
	delete []left_margin;

	CheckForIdenticalTaxonNames();

	return 1;
}

void DataMatrix::DumpCounts( const char* s )
{
	ofstream tmpf( "tmpfile.txt", ios::out | ios::app );
   tmpf << endl << endl;
   if(s) { tmpf << s << endl; }
   for( int j = 0; j < nChar; j++ ) {
      tmpf << j << "  " << Count(j) << endl;
   }
   tmpf << endl;
}

//
// saves data under the name s but with extension changed to '.mlt'
// if third argument supplied, a NEXUS file ending in '.nex' is saved also
//
int DataMatrix::Save( const char* path, char* newfname /* = 0 */, char*
#if defined( AUTOSAVE_NEXUS )
	nxsfname /* = 0 */
#endif
   )
{
	int i, j;//, nchar_total;
	char newpath[ MAXPATH ];

	strcpy( newpath, path );

#if defined( AUTOSAVE_NEXUS )
	//  ________________________________________
	// |                                        |
	// | save uncompressed data to file nxspath |
	// |________________________________________|
	//
   int k;
	char nxspath[ MAXPATH ];
	strcat( nxspath, ".nex" );

	cerr << endl << "Opening file '" << nxspath << "' for saving..." << endl;

	ofstream nxsf( nxspath );
	if( !nxsf ) {
		cerr << endl << "Error: could not open file '" << nxspath << "' for saving" << endl;
		return 0;
	}

	nchar_total = 0;
	for( j = 0; j < nChar; j++ )
		nchar_total += Count(j);

	nxsf << "#nexus" << endl << endl;
	nxsf << "begin data;" << endl;
	nxsf << "  dimensions ntax=" << nTax << "  nchar=" << nchar_total << ";" << endl;
	nxsf << "  format missing=? datatype=standard;" << endl;
	nxsf << "  matrix" << endl;

	for( i = 0; i < nTax; i++ ) {
		nxsf << TaxonLabel(i) << "  ";
		nxsf << " [" << TaxonColor(i) << "]  ";

		for( j = 0; j < nChar; j++ ) {
			for( k = 0; k < Count(j); k++ ) {
				nxsf << DatumToChar( Matrix( i, j ) );
			}
		}
		nxsf << endl;
	}

	nxsf << ";" << endl;
	nxsf << "end;" << endl << endl;

	if( !nxsf ) {
		cerr << endl << "Error saving data to file '" << nxspath << "':  disk full?" << endl;
		return 0;
	}

	nxsf.close();

	if( nxsfname ) {
		strcpy( nxsfname, nxspath );
	}
#endif

	//  _______________________________________
	// |                                       |
	// | save compressed data to file newpath  |
	// |_______________________________________|
	//

	strcat( newpath, ".comp" );
	outman.UserMessage("Opening file \"%s\" for saving...", newpath);

	ofstream outf( newpath );
	if( !outf ) throw ErrorException("Error: could not open file \"%s\"", newpath);

/*	nchar_total = 0;
	for( j = 0; j < nChar; j++ ) {
		int k = PatternType(j);
		if( (k & PT_CONSTANT) && !InvarCharsExpected() ) continue;
		nchar_total++;
	}
*/
	outf << nTax << "  " << nChar << endl;
	for( i = 0; i < nTax; i++ ) {
		outf << TaxonLabel(i) << "  ";
		for( j = 0; j < nChar; j++ ) {
//			int k = PatternType(j);
//			if( (k & PT_CONSTANT) && !InvarCharsExpected() ) continue;
			outf << DatumToChar( Matrix( i, j ) );
		}
		outf << endl;
	}

	// save a line containing the counts for each character
	for( j = 0; j < nChar; j++ ) {
//		int k = PatternType(j);
//		if( (k & PT_CONSTANT) && !InvarCharsExpected() ) continue;
		outf << Count(j) << ' ';
	}
	outf << endl;

	// save a line containing the number of states for each character
	for( j = 0; j < nChar; j++ ) {
//		int k = PatternType(j);
//		if( (k & PT_CONSTANT) && !InvarCharsExpected() ) continue;
		outf << NumStates(j) << ' ';
	}
	outf << endl;

	if( !outf ) {
		cerr << endl << "Error saving data to file '" << newpath << "':  disk full?" << endl;
		return 0;
	}

	outf.close();

	/* cjb
	if( newfname ) {
		strcpy( newfname, newpath );
	}
	*/

	return 1;
}

#if defined( UNUSED )
//
//	CalcNucleotideFreqs computes the simple proportions of bases in a dna
//	data matrix.  BUGBUG ambiguities treated like missing data.
//
void DataMatrix::CalcNucleotideFreqs(FLOAT_TYPE& A, FLOAT_TYPE& C, FLOAT_TYPE& G, FLOAT_TYPE& T)
{
	long total = 0L;
	long nA = 0L;
	long nC = 0L;
	long nG = 0L;
	long nT = 0L;
	for( int k = 0; k < nChar; k++ ) {
		for( int i = 0; i < nTax; i++ ) {
			switch( State(i, k) ) {
				case 1: nA += count[k]; total += count[k]; break;
				case 2: nC += count[k]; total += count[k]; break;
				case 3: nG += count[k]; total += count[k]; break;
				case 4: nT += count[k]; total += count[k]; break;
			}
		}
	}
	if( total > 0L ) {
		A = (FLOAT_TYPE)nA / (FLOAT_TYPE)total;
		C = (FLOAT_TYPE)nC / (FLOAT_TYPE)total;
		G = (FLOAT_TYPE)nG / (FLOAT_TYPE)total;
		T = (FLOAT_TYPE)nT / (FLOAT_TYPE)total;
	}
	else {
		A = 0.25;
		C = 0.25;
		G = 0.25;
		T = 0.25;
	}
}
#endif

#if defined( UNUSED )
//
// SaveAsNexus saves the matrix to the file filename
// iosFlags is ORed to ios::out when opening the output file stream
// e.g., SaveAsNexus("doofus", ios::app) would append to the file doofus
//
void DataMatrix::SaveNexus(const char* filename, int iosFlags /* = 0 */)
{
//	ofstream savf(filename, ios::out | iosFlags ); //DZ
	ofstream savf(filename, ios::out);				//DZ

	if( !iosFlags )
		savf <<   "#nexus\n";
	else
		savf <<   "\n\n";
	savf << "\nbegin data;";
	savf << "\n  dimensions ntax=" << nTax << "  nchar=" << nChar << ";";
	savf << "\n  format datatype=";
	int dataType=1; //DZ
	switch( dataType ) {
		case 0: savf << "standard"; break;
		case 1: savf << "dna"; break;
		case 2: savf << "rna"; break;
		case 3: savf << "protein"; break;
	}
	savf << ";";
	savf << "\n  matrix";

	for( int i = 0; i < nTax; i++ ) {
		savf << "\n  " << TaxonLabel(i) << "  ";
		savf << " [" << TaxonColor(i) << "]  ";
		for( int j = 0; j < nChar; j++ )
			savf << char(Matrix(i, j));
	}

	savf << "\n;";
	savf << "\nendblock;\n";

	savf << "\nbegin sets;";
	savf << "\n wtset counts vector = ";
	for( int k = 0; k < nChar; k++ )
		savf << Count(k) << " ";
	savf << ";";
	savf << "\nendblock;\n";

	savf.close();
}
#endif

void DataMatrix::WriteCollapsedData(){
		
//write the data matrix
	for(int i=0;i<nTax;i++){
		}
	


	}


/**********************/
/* serialization code */
/**********************/

int DataMatrix::Serialize(char** buf_, int* size_)	{
	char*& buf = *buf_;
	int& size = *size_;
	int nTemp;
	size = 0;

	// first calculate the size needed for the buffer

	// calc size of all the stack vars
	size =	sizeof(nTax) + sizeof(nChar) + sizeof(dense) + sizeof(nConstant) + sizeof(lastConstant) + sizeof(nInformative) +
			sizeof(nAutapomorphic) + sizeof(dmFlags) + sizeof(maxNumStates) + sizeof(info);

	// calc size of the actual matrix
	int matrix_size = (nTax * nChar) * sizeof(unsigned char);

	// calc size of the count and number array
	int count_size = nChar * sizeof(int);
	int number_size = nChar * sizeof(int);
	
	//calc size of the constBase array
	int constbase_size = (lastConstant +1) * sizeof(int);

	// calc size of the label and color array (including null terminators)
	int label_size = 0, color_size = 0;
	for (int i = 0; i < nTax; ++i)	{
		label_size += (int)strlen(taxonLabel[i]) + 1;
	}

	// calc size of the stateDistr array
	int stateDistr_size;
	if (stateDistr)
		stateDistr_size = (maxNumStates+1) * sizeof(FLOAT_TYPE);
	else
		stateDistr_size = 0;

	// calc size of the numStates array
	int numStates_size = nChar * sizeof(int);

	size += matrix_size + count_size + constbase_size + number_size + label_size + color_size + stateDistr_size + numStates_size;

	// we gotta send the size of each serialized data struct before we actually send the serialized data.
	// there are 8 dynamic data structs in this data structure
	//removed colors, so now there are only 7
	//size += 8 * sizeof(int);
	size += 7 * sizeof(int);


	// allocate the buffer
	buf = new char[size];

	// now fill in the buffer

	int bptr = 0;

	// first the statics

	memcpy(buf+bptr, &nTax, sizeof(nTax));
	bptr += sizeof(nTax);

	memcpy(buf+bptr, &nChar, sizeof(nChar));
	bptr += sizeof(nChar);

	memcpy(buf+bptr, &dense, sizeof(dense));
	bptr += sizeof(dense);

	memcpy(buf+bptr, &nConstant, sizeof(nConstant));
	bptr += sizeof(nConstant);

	memcpy(buf+bptr, &lastConstant, sizeof(lastConstant));
	bptr += sizeof(lastConstant);

	memcpy(buf+bptr, &nInformative, sizeof(nInformative));
	bptr += sizeof(nInformative);

	memcpy(buf+bptr, &nAutapomorphic, sizeof(nAutapomorphic));
	bptr += sizeof(nAutapomorphic);

	memcpy(buf+bptr, &dmFlags, sizeof(dmFlags));
	bptr += sizeof(dmFlags);

	memcpy(buf+bptr, &maxNumStates, sizeof(maxNumStates));
	bptr += sizeof(maxNumStates);

	memcpy(buf+bptr, info, sizeof(info));
	bptr += sizeof(info);

	// now copy the dynamic stuff into the buffer, make sure to copy their sizes in first!!  as ints!!!

	memcpy(buf+bptr, &matrix_size, sizeof(matrix_size));
	bptr += sizeof(matrix_size);
	for (int i = 0; i < nTax; ++i)	{
		memcpy(buf+bptr, matrix[i], nChar * sizeof(unsigned char));
		bptr += nChar * sizeof(unsigned char);
	}

	memcpy(buf+bptr, &count_size, sizeof(count_size));
	bptr += sizeof(count_size);
	memcpy(buf+bptr, count, count_size);
	bptr += count_size;

	memcpy(buf+bptr, &constbase_size, sizeof(constbase_size));
	bptr += sizeof(constbase_size);
	memcpy(buf+bptr, constBases, constbase_size);
	bptr += constbase_size;

	memcpy(buf+bptr, &number_size, sizeof(number_size));
	bptr += sizeof(number_size);
	memcpy(buf+bptr, number, number_size);
	bptr += number_size;

	memcpy(buf+bptr, &label_size, sizeof(label_size));
	bptr += sizeof(label_size);
	for (int i = 0; i < nTax; ++i)	{
		nTemp = (int)strlen(taxonLabel[i]) + 1;
		memcpy(buf+bptr, taxonLabel[i], nTemp);
		bptr += nTemp;
	}
/*
	memcpy(buf+bptr, &color_size, sizeof(color_size));
	bptr += sizeof(color_size);
	for (int i = 0; i < nTax; ++i)	{
		nTemp = strlen(taxonColor[i]) + 1;
		memcpy(buf+bptr, taxonColor[i], nTemp);
		bptr += nTemp;
	}
*/
	memcpy(buf+bptr, &stateDistr_size, sizeof(stateDistr_size));
	bptr += sizeof(stateDistr_size);
	memcpy(buf+bptr, stateDistr, stateDistr_size);
	bptr += stateDistr_size;

	memcpy(buf+bptr, &numStates_size, sizeof(numStates_size));
	bptr += sizeof(numStates_size);
	memcpy(buf+bptr, numStates, numStates_size);
	bptr += numStates_size;

	return 0;

}

int DataMatrix::Deserialize(const char* buf, const int size_in)	{

	// clear the matrix
	ExplicitDestructor();

	const char* p = buf;

	// get the stack vars first

	memcpy(&nTax, p, sizeof(nTax));
	p += sizeof(nTax);

	memcpy(&nChar, p, sizeof(nChar));
	p += sizeof(nChar);

	memcpy(&dense, p, sizeof(dense));
	p += sizeof(dense);

	memcpy(&nConstant, p, sizeof(nConstant));
	p += sizeof(nConstant);

	memcpy(&lastConstant, p, sizeof(lastConstant));
	p += sizeof(lastConstant);

	memcpy(&nInformative, p, sizeof(nInformative));
	p += sizeof(nInformative);

	memcpy(&nAutapomorphic, p, sizeof(nAutapomorphic));
	p += sizeof(nAutapomorphic);

	memcpy(&dmFlags, p, sizeof(dmFlags));
	p += sizeof(dmFlags);

	memcpy(&maxNumStates, p, sizeof(maxNumStates));
	p += sizeof(maxNumStates);

	memcpy(info, p, sizeof(info));
	p += sizeof(info);

	int size;

	// create the matrix...

	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (nTax > 0)	{
		matrix = new unsigned char*[nTax];
		for (int i = 0; i < nTax; i++)	{
			matrix[i] = new unsigned char[nChar];
			memcpy(matrix[i], p, sizeof(unsigned char) * nChar);
			p += sizeof(unsigned char) * nChar;
		}
	}

	// create the count array...

	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (size > 0)	{
		count = new int[size];
		memcpy(count, p, size);
		p += size;
	}

	// create the constBases array...

	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (size > 0)	{
		constBases = new int[size];
		memcpy(constBases, p, size);
		p += size;
	}

	// create the number array

	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (size > 0)	{
		number = new int[size];
		memcpy(number, p, size);
		p += size;
	}

	// create the label array


	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (size > 0)	{
		taxonLabel = new char*[nTax];
		for (int i = 0; i < nTax; ++i)	{
			int len = (int)strlen(p) + 1;
			taxonLabel[i] = new char[len];
			strcpy(taxonLabel[i], p);
			p += len;
		}
	}

	// create the color array
/*
	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (size > 0)	{
		taxonColor = new char*[nTax];
		for (int i = 0; i < nTax; ++i)	{
			int len = strlen(p) + 1;
			taxonColor[i] = new char[len];
			strcpy(taxonColor[i], p);
			p += len;
		}
	}
*/
	// create the state distribution array

	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (size > 0)	{
		stateDistr = new FLOAT_TYPE[size];
		memcpy(stateDistr, p, size);
		p += size;
	}

	// create the number of states array

	memcpy(&size, p, sizeof(size));
	p += sizeof(size);

	if (size > 0)	{
		numStates = new int[size];
		memcpy(numStates, p, size);
		p += size;
	}

	int diff = (int)(p - buf);
	assert(p-buf == size_in);
	
	return 0;

}


bool DataMatrix::operator==(const DataMatrix& rhs) const	{
	if (&rhs == this)
		return true;

	// test the stack vars

	if (nTax != rhs.nTax || nChar != rhs.nChar	||
		dense != rhs.dense || nConstant != rhs.nConstant	||
		nInformative != rhs.nInformative || nAutapomorphic != rhs.nAutapomorphic	||
		dmFlags != rhs.dmFlags || maxNumStates != rhs.maxNumStates)
		return false;

	if (strcmp(info, rhs.info) != 0)
		return false;

	for (int i = 0; i < nTax; ++i)	{
		if (memcmp(matrix[i], rhs.matrix[i], sizeof(unsigned char) * nChar) != 0)
			return false;
	}

	if (memcmp(count, rhs.count, sizeof(int) * nChar) != 0)
		return false;

	if (memcmp(number, rhs.number, sizeof(int) * nChar) != 0)
		return false;

	for (int i = 0; i < nTax; ++i)	{
		if (strcmp(taxonLabel[i], rhs.taxonLabel[i]) != 0)
			return false;
//		if (strcmp(taxonColor[i], rhs.taxonColor[i]) != 0)
//			return false;
	}

	if (stateDistr != NULL && rhs.stateDistr != NULL)	{
		if (memcmp(stateDistr, rhs.stateDistr, (maxNumStates+1) * sizeof(FLOAT_TYPE)) != 0)
			return false;
	}

	if (memcmp(numStates, rhs.numStates, nChar * sizeof(int)) != 0)
		return false;

	return true;
}
	
void DataMatrix::ExplicitDestructor()	{
	if( count ) MEM_DELETE_ARRAY(count); // count is of length nChar
	if( numStates ) MEM_DELETE_ARRAY(numStates); // numStates is of length nChar
	if( stateDistr ) MEM_DELETE_ARRAY(stateDistr); // stateDistr is of length (maxNumStates+1)
	if( number ) MEM_DELETE_ARRAY(number); // number is of length nChar
	if( taxonLabel ) {
		int j;
		for( j = 0; j < nTax; j++ )
			MEM_DELETE_ARRAY( taxonLabel[j] ); // taxonLabel[j] is of length strlen(taxonLabel[j])+1
	       MEM_DELETE_ARRAY(taxonLabel); // taxonLabel is of length nTax
	}
/*	if( taxonColor ) {
		int j;
		for( j = 0; j < nTax; j++ )
			MEM_DELETE_ARRAY( taxonColor[j] ); // taxonColor[j] is of length strlen(taxonColor[j])+1
	       MEM_DELETE_ARRAY(taxonColor); // taxonColor is of length nTax
	}*/
	if( matrix ) {
		int j;
		for( j = 0; j < nTax; j++ )
			MEM_DELETE_ARRAY(matrix[j]); // matrix[j] is of length nChar
		MEM_DELETE_ARRAY(matrix); // matrix is of length nTax
	}
	memset(this, 0, sizeof(DataMatrix));
}


void DataMatrix::Reweight(FLOAT_TYPE prob){
	for(int i=0;i<nChar;i++){
		FLOAT_TYPE r=rnd.uniform();
		if(r * 2.0 < prob) count[i]++;
		else if(r < prob) count[i]--;
		}
	}

void DataMatrix::BootstrapReweight(){

	RestoreOriginalCounts();
	
	FLOAT_TYPE *cumProbs = new FLOAT_TYPE[nChar];
	
	FLOAT_TYPE p=0.0;
	cumProbs[0]=(FLOAT_TYPE) origCounts[0] / ((FLOAT_TYPE) totalNChar);
	for(int i=1;i<nChar;i++){
		cumProbs[i] = cumProbs[i-1] + (FLOAT_TYPE) origCounts[i] / ((FLOAT_TYPE) totalNChar);
		}
		
	for(int q=0;q<nChar;q++) count[q]=0;

//	ofstream deb("counts.log", ios::app);

	for(int c=0;c<totalNChar;c++){
		FLOAT_TYPE p=rnd.uniform();
		int pat=0; 
		while(p > cumProbs[pat]) pat++;
		count[pat]++;
		}
	int num0=0;
	for(int d=0;d<nChar;d++){
		if(count[d]==0) num0++;
//		deb << count[d] << "\t";
		}
//	deb << endl;
//	deb.close();
	}

void DataMatrix::CheckForIdenticalTaxonNames(){
	char *name1, *name2;
	vector< pair<int, int> > identicals;


	for(int t1=0;t1<nTax-1;t1++){
		for(int t2=t1+1;t2<nTax;t2++){
			name1 = TaxonLabel(t1);
			name2 = TaxonLabel(t2);
			if(strcmp(name1, name2) == 0) identicals.push_back(make_pair(t1, t2));
			}
		}
	
	if(identicals.size() > 0){
		outman.UserMessage("Error! Multiple sequences with same name encountered!:");
		for(vector< pair<int, int> >::iterator it=identicals.begin() ; it < identicals.end() ; it++){
			outman.UserMessage("\t%s : numbers %d and %d", TaxonLabel((*it).first), (*it).first+1, (*it).second+1);
			}
		throw(ErrorException("Terminating.  Please make all sequence names unique!"));
		}
	}
