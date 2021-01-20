#pragma once

#include <adf.h>
#include "system_settings.h"
#include "aie_kernels.h"



#define GetPhase(Start,Step) {\
	taps[Start+7*Step],taps[Start+6*Step],taps[Start+5*Step],taps[Start+4*Step],\
	taps[Start+3*Step],taps[Start+2*Step],taps[Start+Step],taps[Start]}




	const int SHIFT = 0; // to analyze the output generated by impulses at the input
	//const int SHIFT = 15; // for realistic input samples

using namespace adf;



class FIRGraph_SSR4: public adf::graph
{
private:
	kernel k[4][4];
    
    std::vector<cint16> taps = std::vector<cint16>({
    #include "FilterTaps.txt"
    });
    
    std::vector<cint16> taps4_p3 = std::vector<cint16>(GetPhase(0,4));

	std::vector<cint16> taps4_p2 = std::vector<cint16>(GetPhase(1,4));

	std::vector<cint16> taps4_p1 = std::vector<cint16>(GetPhase(2,4));

	std::vector<cint16> taps4_p0 = std::vector<cint16>(GetPhase(3,4));
    


public:
	input_port in[8];
	output_port out[4];

	FIRGraph_SSR4()
	{
		// k[N][0] is always the first in the cascade stream
		// Topology of the TopGraph
		//
		//      3,3   3,2   3,1   3,0 <--
		//  --> 2,0   2,1   2,2   2,3
		//      1,3   1,2   1,1   1,0 <--
		//  --> 0,0   0,1   0,2   0,3
        
        // The Upper-Left triangle must discard a sample (diagonal not included)
		//      3,3   3,2   3,1   3,0 <--
		//  --> 2,0   2,1   2,2   2,3
		//      1,3   1,2   1,1   1,0 <--
		//  --> 0,0   0,1   0,2   0,3

		k[0][0] = kernel::create_object<SingleStream::FIR_MultiKernel_cout<NUM_SAMPLES,SHIFT,0>>(taps4_p0);
		k[0][1] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,0>>(taps4_p1);
		k[0][2] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,0>>(taps4_p2);
		k[0][3] = kernel::create_object<SingleStream::FIR_MultiKernel_cin<NUM_SAMPLES,SHIFT,0>>(taps4_p3);

		k[1][0] = kernel::create_object<SingleStream::FIR_MultiKernel_cout<NUM_SAMPLES,SHIFT,0>>(taps4_p2);
		k[1][1] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,0>>(taps4_p1);
		k[1][2] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,0>>(taps4_p0);
		k[1][3] = kernel::create_object<SingleStream::FIR_MultiKernel_cin<NUM_SAMPLES,SHIFT,1>>(taps4_p3);

		k[2][0] = kernel::create_object<SingleStream::FIR_MultiKernel_cout<NUM_SAMPLES,SHIFT,1>>(taps4_p2);
		k[2][1] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,1>>(taps4_p3);
		k[2][2] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,0>>(taps4_p0);
		k[2][3] = kernel::create_object<SingleStream::FIR_MultiKernel_cin<NUM_SAMPLES,SHIFT,0>>(taps4_p1);

		k[3][0] = kernel::create_object<SingleStream::FIR_MultiKernel_cout<NUM_SAMPLES,SHIFT,0>>(taps4_p0);
		k[3][1] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,1>>(taps4_p3);
		k[3][2] = kernel::create_object<SingleStream::FIR_MultiKernel_cincout<NUM_SAMPLES,SHIFT,1>>(taps4_p2);
		k[3][3] = kernel::create_object<SingleStream::FIR_MultiKernel_cin<NUM_SAMPLES,SHIFT,1>>(taps4_p1);

		const int NPhases = 4;

		for(int i=0;i<NPhases;i++)
			for(int j=0;j<NPhases;j++)
			{
				runtime<ratio>(k[i][j]) = 0.9;
				source(k[i][j]) = "./src/aie_kernels/FirSingleStream.cpp";
				headers(k[i][j]) = {"./src/aie_kernels/FirSingleStream.h"};
			}

		// Constraints: location of the first kernel in the cascade
		for(int i=0;i<NPhases;i++)
		{
			int j = (i%2?28:25); // 25 on even rows and 28 on odd rows
			location<kernel>(k[i][0]) = tile(j,i);
		}



		// Cascade Connections
		for(int row=0;row<NPhases;row++)
		{
			for(int i=0;i<NPhases-1;i++) connect<cascade> (k[row][i].out[0],k[row][i+1].in[1]);
			connect<stream> (k[row][3].out[0],out[row]);
		}

		// Input Streams connections
		for(int row = 0;row<NPhases;row++)
			for(int col=0;col<NPhases;col++)
			{
				int col1 = (row%2?NPhases-col-1:col); // kernel col is inverted on odd rows
				int offset = (row%2);

				connect<stream>(in[2*col+offset],k[row][col1].in[0]);
			}
	};
};


class TopGraph: public adf::graph
{
public:
	FIRGraph_SSR4 G1;

	port<input> in[8];
	port<output> out[4];

	TopGraph()
	{
		for(int i=0;i<4;i++)
		{
			connect<> (in[2*i],G1.in[2*i]);
			connect<> (in[2*i+1],G1.in[2*i+1]);
			connect<> (G1.out[i],out[i]);
		}
	}
};
